Sexain-MemAddrTrace
===================

A simple but optimized Pin tool (Pintool) to collect memory access trace. Records are buffered and compressed before outputted to a compact binary file.

*Usage*

To make Pintool (on Linux):
```
$ make PIN_ROOT=<path to Pin kit> obj-intel64/MemAddrTrace.so
```
To make trace parser and analyser:
```
$ make -f makefile.stat
```
More info about Pin can be found in [here](http://software.intel.com/en-us/articles/pintool).
