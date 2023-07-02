int tolower(int c) { return c | ((unsigned)c - 'A' < 26) << 5; }
