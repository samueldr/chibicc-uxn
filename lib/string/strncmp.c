int strncmp(char *a, char *b, unsigned n) {
  if (n == 0) {
    return 0;
  }
  for (;;) {
    char ac = *a++;
    char bc = *b++;
    if (ac - bc || (!ac && !bc) || !--n) {
      return ac - bc;
    }
  }
}
