// gcc jump_std.c -o jump_std && ./jump_std
#include <setjmp.h>
#include <stdio.h>

int main(void) {
  jmp_buf ctx;          // вместо JumpContext
  volatile int i = 0;   // volatile: так требует стандарт

  setjmp(ctx);          // ← Capture(&ctx): сюда будем возвращаться
  printf("i = %d\n", i);
  i++;

  if (i < 5) {
    longjmp(ctx, 1);    // → Jump(&ctx): прыжок назад
  }

  printf("done\n");
  return 0;
}
