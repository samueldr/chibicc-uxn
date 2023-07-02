int strcmp(char *a, char *b) {
  for (;;) {
    char ac = *a++;
    char bc = *b++;
    if (ac - bc || (!ac && !bc)) {
      return ac - bc;
    }
  }
}
