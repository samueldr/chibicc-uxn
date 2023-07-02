int isxdigit(int c) {
  return (unsigned)c - '0' < 10 || (unsigned)(c | 32) - 'a' < 6;
}
