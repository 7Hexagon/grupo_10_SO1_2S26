import http.server, os, subprocess
from pathlib import Path

class Proxy(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args): pass
 def handle_request(self):
  self.connection.settimeout(10)
  try:
   if self.headers.get('Host') != '127.0.0.1:8787': self.send_error(403);return
   length=int(self.headers.get('Content-Length','0'))
   if length < 0 or length > 2048: self.send_error(413);return
   body=self.rfile.read(length)
   headers=['Host: 127.0.0.1:8787','Connection: close']
   for k in ('Content-Type','Origin','Cookie'):
    value=self.headers.get(k)
    if value:
     if '\r' in value or '\n' in value: self.send_error(400);return
     headers.append(k+': '+value)
   headers.append('Content-Length: '+str(length))
   request=(self.command+' '+self.path+' HTTP/1.0\r\n'+'\r\n'.join(headers)+'\r\n\r\n').encode('ascii')+body
   key=str(Path.home()/'.ssh'/'id_ed25519_lfs')
   proc=subprocess.run(['ssh','-i',key,'-o','IdentitiesOnly=yes','-o','BatchMode=yes','-o','ConnectTimeout=5','-o','HostKeyAlias=192.168.56.110','admin_aegis@192.168.56.111','python3 /home/admin_aegis/ssh_http_forward.py'],input=request,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,timeout=12)
   if proc.returncode or not proc.stdout.startswith(b'HTTP/1.'): self.send_error(502);return
   self.connection.sendall(proc.stdout);self.close_connection=True
  except (OSError,ValueError,subprocess.SubprocessError): self.send_error(502)
 def do_GET(self): self.handle_request()
 def do_POST(self): self.handle_request()

if __name__=='__main__': http.server.ThreadingHTTPServer(('127.0.0.1',8787),Proxy).serve_forever()
