"""
CPU-bound задача: потоки против процессов.

Из-за GIL в каждый момент байткод Python исполняет только один поток,
поэтому потоки на счётной задаче не ускоряют. Процессы — ускоряют:
у каждого свой интерпретатор и свой GIL.
"""
import os
import time
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor

N = 1_000_000
PARTS = 4


def count_primes(lo, hi):
    """Число простых в [lo, hi) — нарочно «в лоб», чтобы грузить процессор."""
    count = 0
    for n in range(max(lo, 2), hi):
        d = 2
        while d * d <= n:
            if n % d == 0:
                break
            d += 1
        else:
            count += 1
    return count


def ranges():
    step = N // PARTS
    return [(i * step, (i + 1) * step) for i in range(PARTS)]


def sequential():
    return sum(count_primes(lo, hi) for lo, hi in ranges())


def with_pool(executor_class):
    with executor_class(max_workers=PARTS) as pool:
        futures = [pool.submit(count_primes, lo, hi) for lo, hi in ranges()]
        return sum(f.result() for f in futures)


def measure(name, fn, *args):
    start = time.perf_counter()
    result = fn(*args)
    print(f"{name:12}: {result} простых, {time.perf_counter() - start:.2f} с")


if __name__ == "__main__":                     # обязательно для ProcessPoolExecutor!
    print(f"ядер: {os.cpu_count()}")
    measure("один поток", sequential)
    measure("4 потока", with_pool, ThreadPoolExecutor)
    measure("4 процесса", with_pool, ProcessPoolExecutor)
