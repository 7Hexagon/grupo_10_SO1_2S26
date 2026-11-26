import importlib.util
import io
import subprocess
import threading
import unittest
from email.message import Message
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

spec = importlib.util.spec_from_file_location('aegis_proxy', Path(__file__).with_name('windows_ssh_proxy.py'))
proxy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proxy)

class ProxyTests(unittest.TestCase):
 def run_request(self, method='GET', host='127.0.0.1:8787', origin=None, extra=(), path='/', response=None, failure=None):
  h=proxy.Proxy.__new__(proxy.Proxy); h.headers=Message(); h.headers.add_header('Host',host)
  if origin is not None: h.headers.add_header('Origin',origin)
  for k,v in extra: h.headers.add_header(k,v)
  h.command=method; h.path=path; h.connection=Mock(); h.rfile=io.BytesIO(b'{}'); h.send_error=Mock()
  if response is None: response=b'HTTP/1.0 200 OK\r\nSet-Cookie: aegis_session=x; HttpOnly; SameSite=Strict; Path=/\r\n\r\n{}'
  result=SimpleNamespace(returncode=0,stdout=response)
  with patch.object(proxy.subprocess,'run',return_value=result,side_effect=failure) as run:
   h.handle_request()
  return h,run
 def test_local_get(self):
  h,run=self.run_request(); self.assertTrue(run.called); self.assertFalse(h.send_error.called)
 def test_unknown_host(self):
  h,run=self.run_request(host='bad.example'); h.send_error.assert_called_with(403); self.assertFalse(run.called)
 def test_unknown_origin(self):
  h,run=self.run_request(method='POST',host='lab.example.test',origin='https://evil.test',extra=[('Content-Length','2')],path='/api/login'); h.send_error.assert_called_with(403); self.assertFalse(run.called)
 def test_post_local(self):
  h,run=self.run_request(method='POST',origin='http://127.0.0.1:8787',extra=[('Content-Length','2')],path='/api/login'); self.assertTrue(run.called); self.assertFalse(h.send_error.called)
 def test_forwarded_headers_are_not_forwarded(self):
  h,run=self.run_request(method='POST',origin='http://127.0.0.1:8787',extra=[('Content-Length','2'),('X-Forwarded-Host','evil.test'),('X-Forwarded-Proto','https')],path='/api/login')
  self.assertNotIn(b'X-Forwarded',run.call_args.kwargs['input']); self.assertFalse(h.send_error.called)
 def test_forwarded_headers_cannot_authorize(self):
  h,run=self.run_request(method='POST',origin='https://evil.test',extra=[('Content-Length','2'),('X-Forwarded-Host','lab.example.test')],path='/api/login'); h.send_error.assert_called_with(403); self.assertFalse(run.called)
 def test_bad_framing(self):
  for extra in ([('Transfer-Encoding','chunked')],[('Content-Length','1'),('Content-Length','2')],[('Content-Length','2049')]):
   h,run=self.run_request(extra=extra); self.assertTrue(h.send_error.called); self.assertFalse(run.called)
 def test_ssh_failure(self):
  h,run=self.run_request(failure=subprocess.TimeoutExpired('ssh',12)); h.send_error.assert_called_with(502)
 def test_concurrency_limit(self):
  server=proxy.BoundedServer.__new__(proxy.BoundedServer); server.slots=threading.BoundedSemaphore(16); server.shutdown_request=Mock()
  for _ in range(16): self.assertTrue(server.slots.acquire(False))
  server.process_request(Mock(),('127.0.0.1',1)); server.shutdown_request.assert_called_once()
  for _ in range(16): server.slots.release()

if __name__=='__main__': unittest.main()
