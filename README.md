# chibicc-uxn

This is Rui Ueyama's [chibicc@historical/old](https://github.com/rui314/chibicc/tree/historical/old), retargeted to compile to [Uxn](https://100r.co/site/uxn.html).

Try `make`, then something like

```sh
# chibicc has no preprocessor, but any C compiler's -E flag will do.
# We use -I. to include uxn.h and -P to eliminate "#" lines from the preprocessor output.
gcc -I. -P -E examples/day3.c -o tmp.c
./chibicc tmp.c > c.tal
uxnasm c.tal c.rom
uxnemu c.rom
```

For a more complex and visually interesting demo, try `examples/star.c`.

See also `make test`, which runs a test suite.

## Details

Integers are either 8-bit (`char`) or 16-bit (`short` or `int`); there is no `long` or `long long`. Currently there are only signed integers. Uxn only provides unsigned integer operations, so the following signed operations have to be emulated with helper functions: sign extension (`char` to `int` conversion), comparison, arithmetic right shift and division/modulo. These last two are particularly expensive. TODO: add unsigned types as an alternative.

There are no floats. Arrays, structs, and enums are supported.

Global variable initializers can be pointers to other globals, but only without an offset, so e.g. `int *b = &a;` and `char *foo = "bar";` work, but not `int *b = &a + 1;`. This is because Uxntal can't express an offset to an absolute reference.

The function names `deo deo2 dei dei2` are "intrinsics" corresponding to the uxn instructions. There is a header `uxn.h` defining their prototypes and some useful wrappers around Varvara APIs.

To set up Varvara event handlers, just define any of the following functions:

- `void on_console(void);`
  - Called when a byte is received. Call `console_read()` or `console_type()` to process it.
- `void on_screen(void);`
  - Called 60 times per second to update the screen.
- `void on_audio1(void);`
  - Called when audio ends on channel 1.
- `void on_audio2(void);`
- `void on_audio3(void);`
- `void on_audio4(void);`
- `void on_controller(void);`
  - Called when a button is pressed or released on the controller or keyboard. Call `controller_button()` or `controller_key()` to process it.
- `void on_mouse(void);`
  - Called when the mouse is moved. Call `mouse_x()`, `mouse_y()` and `mouse_state()` to process it.

They will be automatically hooked before your "main" function. You need not (and must not) call `deo2(&on_console, 0x20)`.
