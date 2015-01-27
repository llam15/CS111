#!/bin/sh

# This comment should be ignored

: : : # this does nothing

if echo hello; then echo world; fi; #outputs hello world on 2 lines

echo hello > hello.txt #outputs to hello.txt

cat < hello.txt > hello2.txt # outputs contents of hello.txt to hello2.txt

echo hello world | less #outputs to less


echo hello word > file

while cat file; do rm file; done

until cat file2; do touch file2; done


(echo I am a subshell)

rm hello.txt hello2.txt file2