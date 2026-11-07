#!/usr/bin/env python3
"""Build the real addon and test connection reuse against controlled HTTP/TLS servers.
Usage: python3 run.py /path/to/OpenFrameworks
Uses a temporary OF console project and a temporary self-signed certificate.
"""
import http.server
import os
from pathlib import Path
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
from urllib.parse import urlsplit

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def setup(self):
        super().setup()
        with self.server.counter_lock:
            self.server.counter += 1
            self.ident = f'{self.server.server_port}:{self.server.counter}'
    def log_message(self, *args):
        pass
    def do_GET(self):
        path = urlsplit(self.path).path
        body = self.ident.encode()
        self.send_response(200)
        if path == '/chunked':
            self.send_header('Transfer-Encoding', 'chunked')
            self.end_headers()
            self.wfile.write(f'{len(body):x}\r\n'.encode() + body + b'\r\n0\r\n\r\n')
            return
        self.send_header('Content-Length', str(1000000 if path in ['/truncate', '/slow'] else len(body)))
        if path == '/close':
            self.send_header('Connection', 'close')
        self.end_headers()
        if path == '/slow':
            try:
                for _ in range(100):
                    self.wfile.write(b'x' * 10000)
                    self.wfile.flush()
                    time.sleep(.03)
            except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
                pass
        else:
            self.wfile.write(body)
            self.wfile.flush()
        if path in ['/truncate', '/idle']:
            if path == '/idle':
                time.sleep(.1)
            self.close_connection = True
    def finish(self):
        try:
            super().finish()
        except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
            pass

class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True
    def __init__(self):
        super().__init__(('127.0.0.1', 0), Handler)
        self.counter = 0
        self.counter_lock = threading.Lock()
    def handle_error(self, request, address):
        pass  # Deliberately disconnected clients are part of these tests.

here = Path(__file__).resolve().parent
addon = here.parent.parent
of_root = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='of-http-reuse-') as tmp:
    project = Path(tmp)
    (project/'src').mkdir()
    shutil.copyfile(here/'main.cpp', project/'src/main.cpp')
    (project/'Makefile').write_text(f'OF_ROOT = {of_root}\ninclude $(OF_ROOT)/libs/openFrameworksCompiled/project/makefileCommon/compile.project.mk\n')
    (project/'addons.make').write_text('ofxPoco\n' + os.path.relpath(addon, project) + '\n')
    env = dict(os.environ, ccache='ccache')
    cpus = str(len(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else os.cpu_count() or 1)
    with (project/'build.log').open('w') as log:
        result = subprocess.run(['make', 'Release', '-j'+cpus, 'JOBS=-j'+cpus], cwd=project, env=env, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((project/'build.log').read_text())
        sys.exit(result.returncode)
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1', '-subj', '/CN=localhost', '-keyout', str(project/'key.pem'), '-out', str(project/'cert.pem')], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    servers = [Server() for _ in range(3)]
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(project/'cert.pem', project/'key.pem')
    servers[2].socket = context.wrap_socket(servers[2].socket, server_side=True)
    for server in servers:
        threading.Thread(target=server.serve_forever, daemon=True).start()
    urls = [f'{"https" if i == 2 else "http"}://127.0.0.1:{s.server_port}' for i, s in enumerate(servers)]
    try:
        result = subprocess.run([str(project/'bin'/project.name)] + urls, cwd=project, env=env, timeout=60)
    finally:
        for server in servers:
            server.shutdown()
            server.server_close()
    sys.exit(result.returncode)
