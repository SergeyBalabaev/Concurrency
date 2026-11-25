"""
wait() как комбинаторы из лекции:
    return_when=ALL_COMPLETED   — All:     ждём все
    return_when=FIRST_COMPLETED — FirstOf: ждём первый

Пример: один и тот же файл есть на трёх «зеркалах» с разной скоростью.
"""
import time
from concurrent.futures import ThreadPoolExecutor, wait, FIRST_COMPLETED, ALL_COMPLETED

MIRRORS = {"mirror-a": 1.5, "mirror-b": 0.4, "mirror-c": 0.9}


def download_from(mirror):
    time.sleep(MIRRORS[mirror])
    return f"данные с {mirror}"


def all_of():
    start = time.perf_counter()
    with ThreadPoolExecutor() as pool:
        futures = [pool.submit(download_from, m) for m in MIRRORS]
        done, not_done = wait(futures, return_when=ALL_COMPLETED)
        print(f"All: готово {len(done)}, осталось {len(not_done)}, "
              f"{time.perf_counter() - start:.1f} с")


def first_of():
    start = time.perf_counter()
    with ThreadPoolExecutor() as pool:
        futures = [pool.submit(download_from, m) for m in MIRRORS]
        done, not_done = wait(futures, return_when=FIRST_COMPLETED)
        winner = done.pop()
        print(f"FirstOf: {winner.result()} через {time.perf_counter() - start:.1f} с; "
              f"ещё работают: {len(not_done)}")
    # выход из with ждёт и остальные задачи!
    print(f"FirstOf: вышли из with через {time.perf_counter() - start:.1f} с")


if __name__ == "__main__":
    all_of()
    first_of()
