int atoi(char *s) {
  int magnitude = 0;
  int sign = 1;
  for (;;) {
    char c = *s;
    if (!(c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' ||
          c == '\v')) {
      break;
    }
    s++;
  }
  if (*s == '+') {
    s++;
  } else if (*s == '-') {
    sign = -1;
    s++;
  }
  for (;;) {
    char c = *s;
    if ('0' <= c && c <= '9') {
      magnitude = magnitude * 10 + (c - '0');
    } else {
      break;
    }
    s++;
  }
  return sign * magnitude;
}
