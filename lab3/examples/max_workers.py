"""
Подбор max_workers для IO-bound задачи.

Сначала запустите сервер в соседнем терминале:
    python3 slow_server.py --limit 8
Потом:
    python3 max_workers.py
"""
import http.client
import time
from concurrent.futures import ThreadPoolExecutor

HOST, PORT = "127.0.0.1", 8000
N = 40                                  # сколько «файлов» скачать


def fetch(n):
    conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
    try:
        conn.request("GET", f"/item/{n}")
        resp = conn.getresponse()
        body = resp.read().decode()
        if resp.status != 200:
            raise RuntimeError(f"item {n}: HTTP {resp.status}")
        return body.strip()
    finally:
        conn.close()


def run(workers):
    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(fetch, range(N)))
    assert len(results) == N
    return time.perf_counter() - start


if __name__ == "__main__":
    for w in (1, 2, 4, 8, 16, 32):
        print(f"max_workers={w:2}: {run(w):5.2f} с")
