"""
Функция и её аргументы уходят в другой процесс через pickle.
pickle умеет передать только функцию, объявленную на верхнем уровне модуля
(дочерний процесс находит её по имени). lambda и вложенные функции — не умеет.

bad():  lambda -> ошибка сериализации (PicklingError или
        «AttributeError: Can't get local object» — зависит от версии)
good(): обычная функция верхнего уровня
"""
from concurrent.futures import ProcessPoolExecutor


def square(x):
    return x * x


def bad():
    with ProcessPoolExecutor(max_workers=2) as pool:
        print(list(pool.map(lambda x: x * x, range(5))))


def good():
    with ProcessPoolExecutor(max_workers=2) as pool:
        print(list(pool.map(square, range(5))))


if __name__ == "__main__":
    good()
    # bad()
