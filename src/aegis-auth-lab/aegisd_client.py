"""Client for the existing aegisd line protocol; no shell or credential storage."""
import re
import socket

_USER = re.compile(r"[A-Za-z0-9_-]{1,32}\Z")
_TOKEN = re.compile(r"[0-9a-f]{64}\Z")
_ROLES = {"operador", "administrador"}

class AegisError(Exception):
    pass

class AegisClient:
    def __init__(self, path="/run/aegisd.sock", timeout=3):
        self.path, self.timeout = path, timeout

    @staticmethod
    def _token(value):
        if not isinstance(value, str) or not _TOKEN.fullmatch(value):
            raise ValueError("invalid token")
        return value

    def _request(self, request):
        if len(request) > 1024 or "\n" in request or "\r" in request:
            raise ValueError("invalid request")
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.settimeout(self.timeout)
                sock.connect(self.path)
                sock.sendall(request.encode("ascii") + b"\n")
                data = bytearray()
                while len(data) < 1024 and not data.endswith(b"\n"):
                    part = sock.recv(min(256, 1024-len(data)))
                    if not part:
                        break
                    data.extend(part)
            if not data.endswith(b"\n") or b"\n" in data[:-1]:
                raise AegisError("invalid service response")
            line = data[:-1].decode("ascii")
            if not (line.startswith("OK ") or line.startswith("ERR ")):
                raise AegisError("invalid service response")
            return line
        except (OSError, UnicodeError) as exc:
            raise AegisError("service unavailable") from exc

    def authenticate(self, username, password):
        if not isinstance(username, str) or not _USER.fullmatch(username):
            raise ValueError("invalid username")
        if not isinstance(password, str):
            raise ValueError("invalid password")
        secret = password.encode("utf-8")
        if not secret or len(secret) > 128 or b"\0" in secret:
            raise ValueError("invalid password")
        return self._request("AUTH " + username + " " + secret.hex())

    def validate(self, token):
        return self._request("VALIDATE " + self._token(token))

    def status(self, token):
        return self._request("STATUS " + self._token(token))

    def revoke(self, actor, target):
        return self._request("REVOKE " + self._token(actor) + " " + self._token(target))
