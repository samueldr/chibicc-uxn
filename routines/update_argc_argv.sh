#!/bin/sh

cat routines/argc_argv.tal \
    | sed 's/( \(.*\) )//g' \
    | tr '\n' ' ' \
    | sed -E 's/[[:space:]]+/ /g' \
    | sed 's/^ |0100 /  /' \
    | sed 's/ $/\n/' \
    | sed 's/ &/\n  \&/g' \
    | sed 's/ @/\n@/g' \
    | sed 's/^\(.*\)$/  printf("\1\\n");/'
