#!/usr/bin/env bash
emu=uxnemu
if [ "$1" == "--cli" ]; then emu=uxncli; shift; fi
FILE=$1
shift
./compile.sh $FILE > tmp.tal && uxnasm tmp.tal tmp.rom && $emu tmp.rom "$@"
