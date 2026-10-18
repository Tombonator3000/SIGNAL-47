# Usage: python3 tools/rangeserve.py SITE_DIR (serves SITE_DIR on 127.0.0.1:8047)
# a static server with byte ranges (GitHub Pages has them; http.server does not)
import http.server, os, re, sys
class H(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a): pass
    def send_head(self):
        r = self.headers.get('Range'); path = self.translate_path(self.path)
        if not r or not os.path.isfile(path): return super().send_head()
        m = re.match(r'bytes=(\d*)-(\d*)', r); size = os.path.getsize(path)
        a = int(m.group(1)) if m.group(1) else 0; b = int(m.group(2)) if m.group(2) else size - 1
        b = min(b, size - 1); f = open(path, 'rb'); f.seek(a)
        self.send_response(206); self.send_header('Content-Type', self.guess_type(path))
        self.send_header('Accept-Ranges', 'bytes'); self.send_header('Content-Range', f'bytes {a}-{b}/{size}')
        self.send_header('Content-Length', str(b - a + 1)); self.end_headers()
        self._left = b - a + 1; return f
    def copyfile(self, src, dst):
        n = getattr(self, '_left', None)
        if n is None: return super().copyfile(src, dst)
        while n > 0:
            c = src.read(min(65536, n))
            if not c: break
            dst.write(c); n -= len(c)
os.chdir(sys.argv[1]); http.server.ThreadingHTTPServer(('127.0.0.1', 8047), H).serve_forever()
