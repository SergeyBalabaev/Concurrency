// promise_linux.cpp — сверка с Linux: std::promise / std::shared_future.
//   g++ -O2 promise_linux.cpp -o promise_linux -pthread
//   strace -f -e trace=futex ./promise_linux
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>
#include <vector>

int main() {
    std::promise<int> p;
    std::shared_future<int> f = p.get_future().share();

    std::vector<std::thread> early;
    for (int i = 0; i < 6; i++)
        early.emplace_back([f] { f.get(); });          // ждут до set_value
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fprintf(stderr, "--- set_value ---\n");
    p.set_value(42);
    for (auto &t : early) t.join();

    fprintf(stderr, "--- late gets start ---\n");
    long sum = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000000; i++) sum += f.get();  // значение уже готово
    auto t1 = std::chrono::steady_clock::now();
    fprintf(stderr, "--- late gets end ---\n");
    printf("sum=%ld, ready get: %.1f ns per call\n", sum,
           std::chrono::duration<double, std::nano>(t1 - t0).count() / 1000000);
}
