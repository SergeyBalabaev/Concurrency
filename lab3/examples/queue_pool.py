"""
queue.Queue + пул: «производитель — потребители» из ЛР2, но рабочие живут в пуле.

Рабочий в цикле берёт задания из очереди, пока не получит «пилюлю» None.
q.join() ждёт, пока на каждое put() не придёт task_done() — это WaitGroup.

bad():  забыли пилюли — рабочие вечно висят в q.get(), with не завершается
        (прервать: Ctrl+C, возможно дважды, или Ctrl+\\)
good(): после q.join() кладём по одной пилюле на рабочего
"""
import queue
import threading
import time
from concurrent.futures import ThreadPoolExecutor

WORKERS = 3


def worker(q, results):
    name = threading.current_thread().name
    done = 0
    while True:
        item = q.get()
        if item is None:                 # пилюля: заданий больше не будет
            q.task_done()
            return done
        time.sleep(0.2)                  # «обработка»
        results.put((item, item * item))
        done += 1
        q.task_done()


def run(send_pills):
    q = queue.Queue(maxsize=5)           # ограниченная: производитель не убежит вперёд
    results = queue.Queue()
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = [pool.submit(worker, q, results) for _ in range(WORKERS)]
        for item in range(20):
            q.put(item)                  # блокируется, если в очереди уже 5
        q.join()                         # все 20 заданий обработаны
        print(f"обработано {results.qsize()} заданий")
        if send_pills:
            for _ in range(WORKERS):
                q.put(None)
        else:
            print("пилюли не отправили — сейчас зависнем...")
    print("каждый рабочий сделал:", [f.result() for f in futures])


def bad():
    run(send_pills=False)


def good():
    run(send_pills=True)


if __name__ == "__main__":
    good()
    # bad()
