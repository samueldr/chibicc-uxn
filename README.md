# chibicc-uxn

This is Rui Ueyama's [chibicc@historical/old](https://github.com/rui314/chibicc/tree/historical/old), retargeted to compile C to [Uxntal](https://100r.co/site/uxn.html).

![image](https://github.com/lynn/chibicc/assets/16232127/312c01e7-0a82-43c0-86ef-6215010cd250)

## Usage

Running `make` will build `./chibicc`.

chibicc itself has no preprocessor, but any C compiler's `-E` flag will do. We use `-I.` to include uxn.h and `-P` to eliminate `#` lines from the preprocessor output:

```sh
gcc -I. -P -E examples/day3.c -o tmp.c
./chibicc -O1 tmp.c > tmp.tal
uxnasm tmp.tal tmp.rom
uxnemu tmp.rom
```

The `-O1` or `-O` flag enables the optimization pass. If the flag is omitted, this is equivalent to `-O0` (no optimization).

For a more complex and visually interesting demo, try `examples/star.c`.

See also `make test`, which runs a test suite.

## Supported

* Boring C89 stuff: functions, loops, global and local variables, arrays, pointers, structs, enums.
* `char` (8 bits), `short` (16 bits), `int` (16 bits), `unsigned`.
* Emulation of signed comparisons, signed division and arithmetic right shift.
  * But these are slow, so use `unsigned` if you want fast code.
* Variable argument functions, in a very limited fashion — see `examples/printf.c`.
* Simple peephole optimizations, like `#0004 MUL2` → `#20 SFT2` or `#0004 0005 ADD2` → `#0009`.

## Not supported

* The preprocessor.
* `long` (32-bit integers), `long long` (64-bit integers), `float` or `double`.
* Function pointers.
* Variable-length arrays.
* Bit fields.
* Functions that take or return structs.

## Varvara

To set up Varvara event handlers, just define any of the following functions:

- `void on_console(void);`
  - Called when a byte is received.
  - Use `console_read()` or `console_type()` to process the event.
- `void on_screen(void);`
  - Called 60 times per second to update the screen.
- `void on_audio1(void);`
  - Called when audio ends on channel 1.
- `void on_audio2(void);`
  - Called when audio ends on channel 2.
- `void on_audio3(void);`
  - Called when audio ends on channel 3.
- `void on_audio4(void);`
  - Called when audio ends on channel 4.
- `void on_controller(void);`
  - Called when a button is pressed or released on the controller, or a keyboard key is pressed.
  - Use `controller_button()` or `controller_key()` to process the event.
- `void on_mouse(void);`
  - Called when the mouse is moved.
  - Use `mouse_x()`, `mouse_y()` and `mouse_state()` to process the event.

If they are defined, the compiled startup code will hook them up to the right devices before calling your `main`.
