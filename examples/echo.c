#include <uxn.h>

void on_controller(void) {
  putchar(controller_key());
  putchar('\n');
}

void main() {}
