#ifndef TIMING_H
#define TIMING_H

// RISC-V rdtime: счётчик реального времени.
// В QEMU virt частота 10 МГц: 1 тик = 100 нс.
static inline uint64
r_time(void)
{
  uint64 x;
  asm volatile("rdtime %0" : "=r" (x));
  return x;
}

#define TICKS_PER_US  10
#define TICKS_PER_MS  10000

#endif