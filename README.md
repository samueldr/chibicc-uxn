# chibicc-uxn

This is Rui Ueyama's [chibicc@historical/old](https://github.com/rui314/chibicc/tree/historical/old), retargeted to compile to [Uxn](https://100r.co/site/uxn.html).

Try `make`, then something like

```sh
# chibicc has no preprocessor, but any C compiler's -E flag will do.
# We use -I. to include uxn.h and -P to eliminate # lines from the preprocessor output.
gcc -I. -P -E examples/day3.c -o tmp.c
./chibicc tmp.c > c.tal
uxnasm c.tal c.rom
uxnemu c.rom
```

See also `make test`, which runs a test suite (currently failing).

## Details

`short` and `int` are both 16 bits; `long` and `long long` are not supported. There are no floats. Arrays, structs, and enums are supported.

Pointers in global variable initializers aren't supported, so e.g. `int *b = &a;` and `char* foo = "bar";` won't work. `char foo[] = "bar";` is fine though.

The function names `deo deo2 dei dei2 brk` are "intrinsics" corresponding to the uxn instructions. There is a header `uxn.h` defining their prototypes and some useful wrappers around Varvara APIs.

You should call `brk();` at the end of a Varvara "vector" (and probably nowhere else):

```c
#include <uxn.h>

void on_controller() {
  // Do things with controller_button() etc...
  brk();
}

void main() {
  set_controller_vector(&on_controller);
}
```
