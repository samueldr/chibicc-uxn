void *memcpy(void *dst, void *src, unsigned n) {
  char *d = dst;
  char *s = src;
  for (; n; n--)
    *d++ = *s++;
  return dst;
}
