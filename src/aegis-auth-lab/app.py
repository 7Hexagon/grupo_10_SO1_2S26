"""Loopback-only demonstration backend. Not suitable for public deployment."""
import json
import os
import socket
import secrets
import time
from http.cookies import SimpleCookie
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import urlsplit
from aegisd_client import AegisClient, AegisError

HOST, PORT = '127.0.0.1', 8787
ORIGIN = f'http://{HOST}:{PORT}'
HERE = Path(__file__).resolve().parent
client = AegisClient('/run/aegis-lab/relay.sock')
sessions = {}
last_login = 0.0

class Handler(BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(5)
    def log_message(self, *_): pass
    def send(self, status, body, content_type='application/json', cookie=None):
        if isinstance(body, dict): body = json.dumps(body).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Security-Policy', "default-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'")
        self.send_header('Content-Length', str(len(body)))
        if cookie: self.send_header('Set-Cookie', cookie)
        self.end_headers(); self.wfile.write(body)
    def safe_host(self):
        return self.headers.get('Host') == f'{HOST}:{PORT}'
    def session(self):
        c=SimpleCookie()
        try: c.load(self.headers.get('Cookie',''))
        except Exception: return None, None
        sid=c.get('aegis_session'); sid=sid.value if sid else None
        if sid not in sessions: return None, None
        if sessions[sid]['until'] <= time.time():
            sessions.pop(sid, None); return None, None
        return sid, sessions[sid]
    def do_GET(self):
        if not self.safe_host(): return self.send(403, {'error':'solicitud rechazada'})
        path=urlsplit(self.path).path
        assets={'/':('index.html','text/html; charset=utf-8'),'/app.js':('app.js','text/javascript; charset=utf-8'),'/style.css':('style.css','text/css; charset=utf-8')}
        if path in assets:
            name,ctype=assets[path]
            return self.send(200,(HERE/name).read_bytes(),ctype)
        if path == '/api/session':
            _,sess=self.session()
            if not sess: return self.send(401,{'error':'sin sesión'})
            try: response=client.validate(sess['token'])
            except AegisError: return self.send(503,{'error':'servicio no disponible'})
            if not response.startswith('OK '): return self.send(401,{'error':response})
            fields=dict(item.split('=',1) for item in response.split()[1:] if '=' in item)
            return self.send(200,{'usuario':fields.get('usuario'),'rol':fields.get('rol'),'vence_unix':fields.get('vence_unix')})
        return self.send(404,{'error':'no encontrado'})
    def do_POST(self):
        global last_login
        if not self.safe_host() or self.headers.get('Origin') != ORIGIN or self.headers.get('Content-Type') != 'application/json':
            return self.send(403,{'error':'solicitud rechazada'})
        try:
            size=int(self.headers.get('Content-Length','-1'))
            if not 0 <= size <= 2048: raise ValueError()
            data=json.loads(self.rfile.read(size))
            if not isinstance(data,dict): raise ValueError()
        except (ValueError,UnicodeError): return self.send(400,{'error':'solicitud inválida'})
        path=urlsplit(self.path).path
        try:
            if path == '/api/login':
                now=time.monotonic()
                if now-last_login<1: return self.send(429,{'error':'intentá nuevamente más tarde'})
                last_login=now
                result=client.authenticate(data.get('usuario'),data.get('clave'))
                if not result.startswith('OK '): return self.send(401,{'error':result})
                fields=dict(item.split('=',1) for item in result.split()[1:] if '=' in item)
                token=fields['token']; until=int(fields['vence_unix'])
                for k in list(sessions):
                    if sessions[k]['until'] <= time.time(): sessions.pop(k,None)
                if len(sessions)>=128: return self.send(503,{'error':'capacidad agotada'})
                sid=secrets.token_urlsafe(32); sessions[sid]={'token':token,'until':until}
                return self.send(200,{'rol':fields.get('rol'),'vence_unix':until},cookie='aegis_session='+sid+'; HttpOnly; SameSite=Strict; Path=/')
            sid,sess=self.session()
            if not sess: return self.send(401,{'error':'sin sesión'})
            if path == '/api/revoke':
                result=client.revoke(sess['token'],sess['token'])
                if result == 'OK revocada':
                    sessions.pop(sid,None)
                    return self.send(200,{'resultado':result},cookie='aegis_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0')
                return self.send(403,{'error':result})
            if path == '/api/logout':
                sessions.pop(sid,None)
                return self.send(200,{'resultado':'sesión web cerrada; la credencial expira según aegisd'},cookie='aegis_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0')
            return self.send(404,{'error':'no encontrado'})
        except (AegisError,ValueError,KeyError): return self.send(503,{'error':'servicio no disponible o respuesta inválida'})

class UnixHTTPServer(HTTPServer):
    address_family = socket.AF_UNIX

if __name__=='__main__':
    if os.environ.get('LISTEN_PID') != str(os.getpid()) or os.environ.get('LISTEN_FDS') != '1':
        raise SystemExit('requires one systemd-activated Unix socket')
    server = UnixHTTPServer((HOST,PORT), Handler, bind_and_activate=False)
    server.socket.close()
    server.socket = socket.socket(fileno=3)
    if server.socket.family != socket.AF_UNIX or not server.socket.getsockopt(socket.SOL_SOCKET, socket.SO_ACCEPTCONN):
        raise SystemExit('unexpected socket')
    server.server_name = HOST
    server.server_port = PORT
    server.serve_forever()
