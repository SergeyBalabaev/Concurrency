CFLAGS = -O0 -g -Wall -pthread
TSAN   = -O0 -g -Wall -pthread -fsanitize=thread
PROGS  = 01_race 02_mutex 03_condvar 04_semaphore 05_rwlock 06_barrier

# Обычная сборка — без оптимизации (-O0), как в тексте методички:
# с -O2 компилятор может свернуть цикл в 01_race в одно сложение.
all: $(PROGS)

%: %.c
	gcc $(CFLAGS) $< -o $@

# make tsan → 01_race-tsan, 02_mutex-tsan, ...
tsan: $(PROGS:%=%-tsan)

%-tsan: %.c
	gcc $(TSAN) $< -o $@

clean:
	rm -f $(PROGS) $(PROGS:%=%-tsan)

.PHONY: all tsan clean
