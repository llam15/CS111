#!/bin/bash

echo 'aaa' | ./osprdaccess -w 3 -l -d 2 & 
echo 'bbb' | ./osprdaccess -w 3 -l -d 2
sleep 1 && ps aux | grep osprdaccess