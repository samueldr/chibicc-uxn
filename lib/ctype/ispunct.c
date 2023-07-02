int ispunct(int c) {
  return (unsigned)c - '!' < 15 || (unsigned)c - ':' < 7 ||
         (unsigned)c - '[' < 6 || (unsigned)c - '{' < 5;
}
