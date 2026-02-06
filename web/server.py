#!/usr/bin/env python3
import http.server
import sys
import urllib.parse

class H(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlsplit(self.path)
        path = parsed.path
        if path == '/calculator-os' or path == '/calculator-os/':
            path = '/index.html'
        elif path.startswith('/calculator-os/'):
            path = '/' + path[len('/calculator-os/'):]
        if path != parsed.path:
            query = f'?{parsed.query}' if parsed.query else ''
            self.path = f'{path}{query}'
        return super().do_GET()

    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
print(f'http://localhost:{port}')
http.server.HTTPServer(('', port), H).serve_forever()
