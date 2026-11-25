"""
Кооперативная отмена через threading.Event.

Остановить поток «снаружи» в Python нельзя — можно только попросить.
Рабочий сам регулярно проверяет флаг и выходит.

bad():  рабочий флаг не проверяет — просьбу остановиться игнорирует
good(): рабочий проверяет stop и спит через stop.wait(), а не time.sleep()
"""
import threading
import time
from concurrent.futures import ThreadPoolExecutor

start = time.perf_counter()
print_lock = threading.Lock()            # чтобы строки из разных потоков не перемешивались


def log(msg):
    with print_lock:
        print(f"[{time.perf_counter() - start:4.1f} с] {msg}")


def worker_bad(name, stop):
    for step in range(10):
        time.sleep(0.5)                       # 10 шагов по 0.5 с — ничего не проверяем
    log(f"{name}: доработал до конца")
    return name


def worker_good(name, stop):
    for step in range(10):
        if stop.is_set():
            log(f"{name}: получил стоп на шаге {step}, выхожу")
            return None
        stop.wait(0.5)                        # как sleep, но просыпается сразу при set()
    log(f"{name}: доработал до конца")
    return name


def run(worker):
    stop = threading.Event()
    with ThreadPoolExecutor(max_workers=3) as pool:
        futures = [pool.submit(worker, f"w{i}", stop) for i in range(3)]
        time.sleep(1.2)
        log("main: хватит, останавливаемся")
        stop.set()
    log(f"main: все остановились, результаты: {[f.result() for f in futures]}")


if __name__ == "__main__":
    run(worker_good)
    # run(worker_bad)
