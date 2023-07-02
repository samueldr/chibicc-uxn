int toupper(int c) { return c - (((unsigned)c - 'a' < 26) << 5); }
