CFLAGS = -O2 -g -Wall -pthread
TSAN   = -O1 -g -Wall -pthread -fsanitize=thread
PROGS  = bank buffer life

all: $(PROGS)

%: %.c
	gcc $(CFLAGS) $< -o $@

# make tsan → bank-tsan, buffer-tsan, life-tsan
tsan: $(PROGS:%=%-tsan)

%-tsan: %.c
	gcc $(TSAN) $< -o $@

clean:
	rm -f $(PROGS) $(PROGS:%=%-tsan)

.PHONY: all tsan clean
