#!/usr/bin/env bash
cc -I. -P -E -x c "$1" -o tmp.c
./chibicc tmp.c
