#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

# Copyright 2012-2014 Paul Eggert, Kevin Balke, Leslie Lam.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
#!/bin/sh

# This comment should be ignored

: : : # this does nothing

if echo hello; then echo world; fi; #outputs hello world on 2 lines

echo hello > hello.txt #outputs to hello.txt

cat < hello.txt > hello2.txt # outputs contents of hello.txt to hello2.txt

echo hello world | cat #outputs to cat

echo hello word > file

while (test ! -s testfile.txt); do echo hello > testfile.txt; done

until (test ! -s testfile.txt); do rm testfile.txt; done


(echo I am a subshell)

rm hello.txt hello2.txt file
EOF

cat >test.exp <<'EOF'
hello
world
hello world
I am a subshell
EOF

../profsh test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
