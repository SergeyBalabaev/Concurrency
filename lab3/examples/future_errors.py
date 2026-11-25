"""
Исключение внутри задачи не падает в потоке пула — оно сохраняется
в Future и выбрасывается заново при вызове result().

bad():  результат не забираем — ошибка пропадает молча
good(): забираем result() — ошибку видно и можно обработать
"""
import time
from concurrent.futures import ThreadPoolExecutor


def task(n):
    time.sleep(0.1)
    if n == 3:
        raise ValueError(f"задача {n} сломалась")
    return n * n


def bad():
    with ThreadPoolExecutor(max_workers=4) as pool:
        for n in range(6):
            pool.submit(task, n)          # Future выбросили — ошибку никто не увидит
    print("bad: всё прошло «успешно»?")


def good():
    with ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(task, n) for n in range(6)]
        for n, f in enumerate(futures):
            try:
                print(f"задача {n}: результат {f.result()}")
            except ValueError as e:
                print(f"задача {n}: ошибка — {e}")


if __name__ == "__main__":
    #good()
    bad()
