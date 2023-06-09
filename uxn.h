// Raw interface to uxn's "devices"
void deo(char device, char data);
void deo2(char device, int data);
char dei(char device);
int dei2(char device);
void brk();

// https://wiki.xxiivv.com/site/varvara.html
#define palette(r, g, b) (deo2(0x08, r), deo2(0x0a, g), deo2(0x0c, b))
#define putchar(c) deo(0x18, c)
#define set_screen_size(width, height) (deo2(0x22, width), deo2(0x24, height))
#define set_spr_auto(a) deo(0x26, a)
#define set_spr_x(x) deo2(0x28, x)
#define set_spr_y(y) deo2(0x2a, y)
#define spr_x() dei2(0x28)
#define spr_y() dei2(0x2a)
#define set_spr_addr(a) deo2(0x2c, a)
#define draw_pixel(a) deo(0x2e, a)
#define draw_sprite(a) deo(0x2f, a)

#define set_controller_vector(a) deo2(0x80, a)
#define controller_button() dei(0x82)
#define controller_key() dei(0x83)

// Pixel values (| with color number)
// (Layer + operation + corner)
#define BgDot 0x00
#define BgFillBR 0x80
#define BgFillBL 0x90
#define BgFillTR 0xa0
#define BgFillTL 0xb0
#define FgDot 0x40
#define FgFillBR 0xc0
#define FgFillBL 0xd0
#define FgFillTR 0xe0
#define FgFillTL 0xf0

// Sprite values (| with blend number)
// (Layer + bit depth + flip axes)
#define Bg1 0x00
#define Bg1X 0x10
#define Bg1Y 0x20
#define Bg1XY 0x30
#define Fg1 0x40
#define Fg1X 0x50
#define Fg1Y 0x60
#define Fg1XY 0x70
#define Bg2 0x80
#define Bg2X 0x90
#define Bg2Y 0xa0
#define Bg2XY 0xb0
#define Fg2 0xc0
#define Fg2X 0xd0
#define Fg2Y 0xe0
#define Fg2XY 0xf0

// Auto values
#define Auto1 0x00
#define Auto1x 0x01
#define Auto1y 0x02
#define Auto1a 0x04
#define Auto1ax 0x05
#define Auto1ay 0x06
#define Auto2 0x10
#define Auto2x 0x11
#define Auto2y 0x12
#define Auto2a 0x14
#define Auto2ax 0x15
#define Auto2ay 0x16

// Button values
#define ButtonCtrl 0x01
#define ButtonA 0x01
#define ButtonAlt 0x02
#define ButtonB 0x02
#define ButtonShift 0x04
#define ButtonSelect 0x04
#define ButtonStart 0x08
#define ButtonHome 0x08
#define ButtonUp 0x10
#define ButtonDown 0x20
#define ButtonLeft 0x40
#define ButtonRight 0x80
