#include <uxn.h>

char square[8] = {0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff};

void on_controller() {
  char button = controller_button();
  draw_sprite(button & ButtonCtrl ? 4 : 1);
  if (button & ButtonUp) set_spr_y(spr_y() - 8);
  if (button & ButtonDown) set_spr_y(spr_y() + 8);
  if (button & ButtonLeft) set_spr_x(spr_x() - 8);
  if (button & ButtonRight) set_spr_x(spr_x() + 8);
  brk();
}

void main() {
  palette(0x2ce9, 0x01c0, 0x7ce5);
  set_controller_vector(&on_controller);
  set_spr_x(8);
  set_spr_y(8);
  set_spr_addr(square);
}
