#!/usr/bin/env bash
./compile.sh $1 > tmp.tal && uxnasm tmp.tal tmp.rom && uxnemu tmp.rom
