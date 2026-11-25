"""
Процессам задачи и результаты передаются через pickle по каналу.
Если задача крошечная, пересылка стоит дороже самой работы.

bad():  по одному числу на задачу — сотни тысяч пересылок
good(): крупные куски через chunksize
"""
import time
from concurrent.futures import ProcessPoolExecutor

N = 100_000


def is_prime(n):
    if n < 2:
        return False
    d = 2
    while d * d <= n:
        if n % d == 0:
            return False
        d += 1
    return True


def sequential():
    return sum(map(is_prime, range(N)))


def bad():
    with ProcessPoolExecutor(max_workers=4) as pool:
        return sum(pool.map(is_prime, range(N)))          # chunksize=1 по умолчанию


def good():
    with ProcessPoolExecutor(max_workers=4) as pool:
        return sum(pool.map(is_prime, range(N), chunksize=5_000))


def measure(name, fn):
    start = time.perf_counter()
    result = fn()
    print(f"{name:14}: {result} простых, {time.perf_counter() - start:.2f} с")


if __name__ == "__main__":
    measure("один поток", sequential)
    measure("chunksize=1", bad)
    measure("chunksize=5000", good)
