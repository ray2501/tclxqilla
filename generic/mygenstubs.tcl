#!/usr/bin/tclsh

set variable [list tclsh ../tools/genExtStubs.tcl xqillaStubDefs.txt xqillaStubs.h xqillaStubInit.c]
exec {*}$variable
