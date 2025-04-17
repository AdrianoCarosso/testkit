#!/bin/sh
# readelf -sW ./libc.so.6 | grep GLIBC_ABI_DT_RELR

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib; ./nrfjprog
#/lib64/ld-linux-x86-64.so.2 --library-path /usr/local/lib  ./nrfjprog
/usr/local/lib/ld-linux-x86-64.so.2 --library-path /usr/local/lib  ./nrfjprog

