"""
Локальный «медленный» HTTP-сервер для экспериментов (интернет не нужен).

    python3 slow_server.py                       # порт 8000, задержка 0.3 с, не более 8 запросов разом
    python3 slow_server.py --port 8001 --delay 1.0 --jitter 0.5 --limit 4 --fail 0.1

GET /item/<n>  -> через delay (+ случайно до jitter) секунд отвечает строкой "item <n>"
    --limit K  сервер обрабатывает не больше K запросов одновременно, остальные ждут
    --fail p   с вероятностью p отвечает ошибкой 500
"""
import argparse
import random
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        parts = self.path.strip("/").split("/")
        if len(parts) != 2 or parts[0] != "item" or not parts[1].isdigit():
            self.send_error(404, "use /item/<n>")
            return

        with self.server.slots:                       # не больше limit одновременно
            time.sleep(self.server.delay + random.uniform(0, self.server.jitter))

        if random.random() < self.server.fail:
            self.send_error(500, "random failure")
            return

        body = f"item {parts[1]}\n".encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):                # чтобы не засорять консоль
        if not self.server.quiet:
            super().log_message(fmt, *args)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=8000)
    p.add_argument("--delay", type=float, default=0.3)
    p.add_argument("--jitter", type=float, default=0.0)
    p.add_argument("--limit", type=int, default=8)
    p.add_argument("--fail", type=float, default=0.0)
    p.add_argument("--verbose", action="store_true")
    a = p.parse_args()

    ThreadingHTTPServer.request_queue_size = 256
    srv = ThreadingHTTPServer((a.host, a.port), Handler)
    srv.daemon_threads = True
    srv.delay, srv.jitter, srv.fail = a.delay, a.jitter, a.fail
    srv.slots = threading.Semaphore(a.limit)
    srv.quiet = not a.verbose
    print(f"http://{a.host}:{a.port}/item/<n>  delay={a.delay} jitter={a.jitter} "
          f"limit={a.limit} fail={a.fail}  (Ctrl+C — остановить)")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
