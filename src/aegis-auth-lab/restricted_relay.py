"""Restricted Unix-socket relay. Requires OS sandboxing and a dedicated UID.

Not installed or enabled. No arbitrary commands, bootstrap, or user creation.
"""
import os
import re
import socket
import struct
import time

_HEX64 = r"[0-9a-f]{64}"
_ALLOWED = re.compile(r"(?:AUTH [A-Za-z0-9_-]{1,32} (?:[0-9a-f]{2}){1,128}|(?:VALIDATE|STATUS) " + _HEX64 + r"|REVOKE " + _HEX64 + " " + _HEX64 + r")\Z")


def one_line(conn, limit=1024):
    conn.settimeout(3)
    data = bytearray()
    while len(data) < limit:
        chunk = conn.recv(min(256, limit - len(data)))
        if not chunk:
            break
        data.extend(chunk)
        if b'\n' in data:
            break
    if not data.endswith(b'\n') or b'\n' in data[:-1] or b'\r' in data:
        raise ValueError('invalid line')
    return data[:-1]


def relay_one(conn, daemon_path, expected_uid):
    try:
        _pid, uid, _gid = struct.unpack('3i', conn.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, struct.calcsize('3i')))
        if uid != expected_uid:
            return
        request = one_line(conn).decode('ascii')
        if not _ALLOWED.fullmatch(request):
            conn.sendall(b'ERR solicitud\n')
            return
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as daemon:
            daemon.settimeout(3)
            daemon.connect(daemon_path)
            daemon.sendall(request.encode('ascii') + b'\n')
            answer = one_line(daemon, 1024)
        if not (answer.startswith(b'OK ') or answer.startswith(b'ERR ')):
            raise ValueError('invalid daemon response')
        conn.sendall(answer + b'\n')
    except (OSError, UnicodeError, ValueError):
        try: conn.sendall(b'ERR servicio\n')
        except OSError: pass


def serve(listener, daemon_path, expected_uid):
    # Serial operation also protects the single-client aegisd from concurrency.
    previous = 0.0
    while True:
        conn, _ = listener.accept()
        with conn:
            now = time.monotonic()
            if now - previous < 0.1:
                conn.sendall(b'ERR limite\n')
                continue
            previous = now
            relay_one(conn, daemon_path, expected_uid)

if __name__ == '__main__':
    import pwd
    import sys
    if os.geteuid() != 0 or os.environ.get('LISTEN_PID') != str(os.getpid()) or os.environ.get('LISTEN_FDS') != '1':
        sys.exit('requires root and one systemd-activated socket')
    try:
        uid = pwd.getpwnam('aegislab').pw_uid
        if uid == 0:
            raise ValueError('invalid service UID')
        with socket.socket(fileno=3) as listener:
            if listener.family != socket.AF_UNIX or listener.type & socket.SOCK_STREAM == 0:
                raise ValueError('unexpected socket')
            serve(listener, '/run/aegisd.sock', uid)
    except (KeyError, ValueError, OSError):
        sys.exit('relay configuration error')
