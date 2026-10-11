// gcc -O0 jump.c -o jump && ./jump
#include <stdio.h>

typedef struct {
  void* rsp;  // каким будет стек после возврата из Capture
  void* rip;  // куда возвращаться из Capture
} JumpContext;

void Capture(JumpContext* ctx);  // запомнить «где я»
void Jump(JumpContext* ctx);     // вернуться туда

__asm__(
    "Capture:\n"
    "  movq (%rsp), %rax\n"    // rax = адрес возврата (его положил call)
    "  movq %rax, 8(%rdi)\n"   // ctx->rip = rax
    "  leaq 8(%rsp), %rax\n"
    "  movq %rax, 0(%rdi)\n"   // ctx->rsp = rsp после ret
    "  ret\n"

    "Jump:\n"
    "  movq 0(%rdi), %rsp\n"   // rsp = ctx->rsp
    "  pushq 8(%rdi)\n"        // положить на стек ctx->rip
    "  ret\n"                  // ret «вернёт» нас из Capture
);

int main(void) {
  JumpContext ctx;
  int i = 0;

  Capture(&ctx);   // ← сюда будем возвращаться
  printf("i = %d\n", i);
  i++;

  if (i < 5) {
    Jump(&ctx);    // → прыжок назад, к строке с Capture
  }

  printf("done\n");
  return 0;
}