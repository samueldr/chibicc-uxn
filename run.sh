#!/usr/bin/env bash
emu=uxnemu
if [ "$1" == "--cli" ]; then emu=uxncli; shift; fi
./compile.sh $1 > tmp.tal && uxnasm tmp.tal tmp.rom && $emu tmp.rom
