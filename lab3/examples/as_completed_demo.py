"""
map отдаёт результаты в порядке ПОДАЧИ задач,
as_completed — в порядке ГОТОВНОСТИ.
"""
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

DELAYS = [2.0, 0.5, 1.0, 0.2]           # задача 0 самая медленная


def work(n):
    time.sleep(DELAYS[n])
    return n


def with_map():
    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=4) as pool:
        for n in pool.map(work, range(4)):
            print(f"map: задача {n} готова через {time.perf_counter() - start:.1f} с")


def with_as_completed():
    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=4) as pool:
        futures = {pool.submit(work, n): n for n in range(4)}   # Future -> номер задачи
        for f in as_completed(futures):
            n = futures[f]
            print(f"as_completed: задача {n} готова через {time.perf_counter() - start:.1f} с")


if __name__ == "__main__":
    with_map()
    print()
    with_as_completed()
