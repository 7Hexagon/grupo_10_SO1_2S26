import http.server, subprocess, threading
from pathlib import Path

LOCAL_HOST = '127.0.0.1:8787'
LOCAL_ORIGIN = 'http://' + LOCAL_HOST

class Proxy(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args): pass
 def handle_request(self):
  self.connection.settimeout(10)
  try:
   hosts=self.headers.get_all('Host', [])
   if len(hosts)!=1 or hosts[0] != LOCAL_HOST: self.send_error(403);return
   origin=LOCAL_ORIGIN
   origins=self.headers.get_all('Origin', [])
   if (self.command=='POST' and origins != [origin]) or (origins and origins != [origin]): self.send_error(403);return
   if self.path not in (('/', '/app.js', '/style.css', '/api/session') if self.command=='GET' else ('/api/login','/api/logout','/api/revoke')): self.send_error(404);return
   if self.headers.get_all('Transfer-Encoding', []): self.send_error(400);return
   if len(self.headers.get_all('Content-Length', [])) > 1: self.send_error(400);return
   if any(len(self.headers.get_all(k, [])) > 1 for k in ('Origin','Content-Type','Cookie')): self.send_error(400);return
   if self.command == 'POST' and not self.headers.get_all('Content-Length', []): self.send_error(411);return
   length=int(self.headers.get('Content-Length','0'))
   if length < 0 or length > 2048: self.send_error(413);return
   if self.command == 'GET' and length != 0: self.send_error(400);return
   body=self.rfile.read(length)
   if len(body)!=length: self.send_error(400);return
   headers=['Host: 127.0.0.1:8787','Connection: close']
   for k in ('Content-Type','Origin','Cookie'):
    value=LOCAL_ORIGIN if k=='Origin' and origins else self.headers.get(k)
    if value:
     if '\r' in value or '\n' in value: self.send_error(400);return
     headers.append(k+': '+value)
   headers.append('Content-Length: '+str(length))
   request=(self.command+' '+self.path+' HTTP/1.0\r\n'+'\r\n'.join(headers)+'\r\n\r\n').encode('ascii')+body
   if len(request)>8192: self.send_error(413);return
   key=str(Path.home()/'.ssh'/'id_ed25519_lfs')
   proc=subprocess.run(['ssh','-i',key,'-o','IdentitiesOnly=yes','-o','BatchMode=yes','-o','ConnectTimeout=5','-o','HostKeyAlias=192.168.56.110','admin_aegis@192.168.56.111','python3 /home/admin_aegis/ssh_http_forward.py'],input=request,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,timeout=12)
   if proc.returncode or not proc.stdout.startswith(b'HTTP/1.'): self.send_error(502);return
   response=proc.stdout
   if len(response)>1048576 or b'\r\n\r\n' not in response: self.send_error(502);return
   self.connection.sendall(response);self.close_connection=True
  except (OSError,ValueError,UnicodeError,subprocess.SubprocessError):
   try: self.send_error(502)
   except OSError: pass
 def do_GET(self): self.handle_request()
 def do_POST(self): self.handle_request()

class BoundedServer(http.server.ThreadingHTTPServer):
 daemon_threads = True
 def __init__(self, address, handler):
  self.slots = threading.BoundedSemaphore(16)
  super().__init__(address, handler)
 def process_request(self, request, client_address):
  if not self.slots.acquire(blocking=False):
   self.shutdown_request(request); return
  try: super().process_request(request, client_address)
  except BaseException:
   self.slots.release(); raise
 def process_request_thread(self, request, client_address):
  try: super().process_request_thread(request, client_address)
  finally: self.slots.release()

if __name__=='__main__': BoundedServer(('127.0.0.1',8787),Proxy).serve_forever()
