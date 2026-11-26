#!/usr/bin/python3
import socket,sys
request=sys.stdin.buffer.read(8193)
if len(request)>8192: sys.exit(1)
with socket.socket(socket.AF_UNIX,socket.SOCK_STREAM) as s:
 s.settimeout(5)
 s.connect('/run/aegis-web/http.sock')
 s.sendall(request);s.shutdown(socket.SHUT_WR)
 while True:
  data=s.recv(16384)
  if not data: break
  sys.stdout.buffer.write(data)
