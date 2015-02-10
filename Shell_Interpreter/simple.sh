#!/bin/sh

#echo hello word lalala | cat
#if echo hello; then echo world; fi
#sleep 4

cat /usr/share/dict/words | sort -u >/dev/null
echo hello
echo 1
echo 2
echo 3
#(sleep 1)
if echo hello; then echo world; echo check; echo 1; fi;
(exec echo hello)
