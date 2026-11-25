"""
Что умеет и чего не умеет отмена в concurrent.futures.

cancel_pending():       cancel() срабатывает только для задачи, которая ещё ждёт в очереди
timeout_not_cancel():   result(timeout) перестаёт ЖДАТЬ, но задача продолжает РАБОТАТЬ
"""
import threading
import time
from concurrent.futures import ThreadPoolExecutor, CancelledError

start = time.perf_counter()
print_lock = threading.Lock()            # чтобы строки из разных потоков не перемешивались


def log(msg):
    with print_lock:
        print(f"[{time.perf_counter() - start:4.1f} с] {msg}")


def slow(n):
    log(f"задача {n} началась")
    time.sleep(1)
    log(f"задача {n} закончилась")
    return n


def cancel_pending():
    with ThreadPoolExecutor(max_workers=1) as pool:      # один рабочий — остальные ждут
        futures = [pool.submit(slow, n) for n in range(3)]
        time.sleep(0.1)                                   # задача 0 уже выполняется
        for n, f in enumerate(futures):
            log(f"cancel() задачи {n} -> {f.cancel()}")
        for n, f in enumerate(futures):
            try:
                log(f"результат задачи {n}: {f.result()}")
            except CancelledError:
                log(f"задача {n} отменена")


def timeout_not_cancel():
    with ThreadPoolExecutor(max_workers=1) as pool:
        f = pool.submit(slow, 99)
        try:
            f.result(timeout=0.3)
        except TimeoutError:
            log("result(timeout=0.3): TimeoutError — ждать перестали")
        log(f"cancel() -> {f.cancel()}, задача всё ещё running: {f.running()}")
    log("вышли из with — только когда задача доработала")


if __name__ == "__main__":
    cancel_pending()
    print()
    timeout_not_cancel()
