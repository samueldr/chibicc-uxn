void func_name(args, ...) {
  va_list ap;
  va_start(ap, format);
  int x;
  char *s;
  int width = 4;
  _Bool command = 0;
  while (*format) {
    if (command) {
      switch (*format) {
      case '%':
        put('%');
        command = 0;
        break;
      case 'c':
        put(va_arg(ap, int));
        command = 0;
        break;
      case 's':
        s = va_arg(ap, int);
        while (*s)
          put(*s++);
        command = 0;
        break;
      case 'd':
        x = va_arg(ap, int);
        if (x < 0) {
          put('-');
          x = -x;
        }
        if (x > 9999)
          put(x / 10000 % 10 + '0');
        if (x > 999)
          put(x / 1000 % 10 + '0');
        if (x > 99)
          put(x / 100 % 10 + '0');
        if (x > 9)
          put(x / 10 % 10 + '0');
        put(x % 10 + '0');
        command = 0;
        break;
      case 'x':
      case 'X':
        x = va_arg(ap, int);
        for (int i = width * 4; i;) {
          int h = x >> (i -= 4) & 15;
          put(h + (h > 9 ? *format - 33 : '0'));
        }
        command = 0;
        width = 4;
        break;
      case '1':
      case '2':
      case '3':
      case '4':
        width = *format - '0';
        break;
      default:
        return;
      };
    } else if (*format == '%') {
      command = 1;
    } else {
      put(*format);
    }
    ++format;
  }
}
