CC      = gcc
CFLAGS  = -O2 -g -Wall -Wextra -pthread
TSAN    = -O1 -g -Wall -Wextra -pthread -fsanitize=thread

PROGS   = deadlock philo list lru

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

# сборка под ThreadSanitizer: make tsan → deadlock-tsan, philo-tsan, ...
tsan: $(PROGS:%=%-tsan)

%-tsan: %.c
	$(CC) $(TSAN) $< -o $@

clean:
	rm -f $(PROGS) $(PROGS:%=%-tsan)

.PHONY: all tsan clean
