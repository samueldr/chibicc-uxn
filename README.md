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

See also `make test`, which runs a test suite. It needs a [patched version of uxnasm](allow-more-refs.patch).

## Details

Integers are either 8-bit (`char`) or 16-bit (`short` or `int`); there is no `long` or `long long`. Currently there are only signed integers. Uxn only provides unsigned integer operations, so the following signed operations have to be emulated with helper functions: sign extension (`char` to `int` conversion), comparison, arithmetic right shift and division/modulo. These last two are particularly expensive. TODO: add unsigned types as an alternative.

There are no floats. Arrays, structs, and enums are supported.

Global variable initializers can be pointers to other globals, but only without an offset, so e.g. `int *b = &a;` and `char *foo = "bar";` work, but not `int *b = &a + 1;`. This is because Uxntal can't express an offset to an absolute reference.

The function names `deo deo2 dei dei2` are "intrinsics" corresponding to the uxn instructions. There is a header `uxn.h` defining their prototypes and some useful wrappers around Varvara APIs.

Varvara handlers *must* have names starting with `on_` — this is how the compiler knows to emit `BRK` instead of `JMP2r` at the end. Conversely, regular function names may not start with `on_`.

```c
#include <uxn.h>

void on_controller(void) {
  // Do things with controller_button() etc...
}

void main(void) {
  set_controller_vector(&on_controller);
}
```
