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

See also `make test`, which runs a test suite (currently failing). It needs a patched version of uxnasm:

```patch
diff --git a/src/uxnasm.c b/src/uxnasm.c
index b42ba78..07f6e7f 100644
--- a/src/uxnasm.c
+++ b/src/uxnasm.c
@@ -42 +42 @@ typedef struct {
-       Reference refs[0x800];
+       Reference refs[0x8000];
@@ -182 +182 @@ makereference(char *scope, char *label, char rune, Uint16 addr)
-       if(p.rlen >= 0x800)
+       if(p.rlen >= 0x8000)
```

## Details

Integers are either 8-bit (`char`) or 16-bit (`short` or `int`); there is no `long` or `long long`. Currently there are only signed integers. Uxn only provides unsigned integer operations, so the following signed operations have to be emulated with helper functions: sign extension (`char` to `int` conversion), comparison, arithmetic right shift and division/modulo. These last two are particularly expensive. TODO: add unsigned types as an alternative.

There are no floats. Arrays, structs, and enums are supported.

Global variable initializers can be pointers to other globals, but only without an offset, so e.g. `int *b = &a;` and `char *foo = "bar";` work, but not `int *b = &a + 1;`. This is because Uxntal can't express an offset to an absolute reference.

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
