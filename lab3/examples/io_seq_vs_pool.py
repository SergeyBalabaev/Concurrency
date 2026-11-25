"""
IO-bound задачи: последовательно и через ThreadPoolExecutor.

«Скачивание» имитируется через time.sleep — пока поток спит,
он отпускает GIL, и другие потоки могут работать.
"""
import time
from concurrent.futures import ThreadPoolExecutor


def download(n):
    time.sleep(0.5)                   # «ждём сеть»
    return f"файл {n}"


def sequential():
    start = time.perf_counter()
    results = [download(n) for n in range(10)]
    print(results)
    print(f"последовательно: {time.perf_counter() - start:.2f} с")


def with_pool():
    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=10) as pool:
        results = list(pool.map(download, range(10)))
    print(results)
    print(f"пул из 10 потоков: {time.perf_counter() - start:.2f} с")


if __name__ == "__main__":
    sequential()
    with_pool()
