"""
add_done_callback — аналог Then из лекции: «когда будет готово, вызови это».
Колбэк получает сам Future и выполняется в том потоке, который завершил задачу
(или сразу в текущем потоке, если Future уже готов).
"""
import threading
import time
from concurrent.futures import ThreadPoolExecutor


def download(n):
    time.sleep(0.3 * n)
    return f"файл {n}"


def on_done(future):
    print(f"  колбэк в потоке {threading.current_thread().name}: {future.result()}")


if __name__ == "__main__":
    with ThreadPoolExecutor(max_workers=3) as pool:
        for n in range(1, 4):
            f = pool.submit(download, n)
            f.add_done_callback(on_done)
        print("main: задачи отправлены, main не блокируется")
    print("main: все задачи завершены")
