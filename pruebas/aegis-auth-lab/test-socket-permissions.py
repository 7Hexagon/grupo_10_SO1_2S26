import socket
for path in ['/run/aegis-web/http.sock','/run/aegis-lab/relay.sock','/run/aegisd.sock']:
 s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.settimeout(3)
 try:
  s.connect(path)
 except PermissionError: print(path,'DENIED')
 else: raise SystemExit('FAIL: unexpected access to '+path)
 finally: s.close()
print('UNPRIVILEGED_SOCKET_ACCESS_DENIED')
