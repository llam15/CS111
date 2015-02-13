#!/bin/bash

echo 'aaa' | ./osprdaccess -w 3 -l -d 0.2 & 
sleep 0.1 && echo 'bbb' | ./osprdaccess -w 3 -l