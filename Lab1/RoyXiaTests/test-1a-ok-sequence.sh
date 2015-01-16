#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

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

# Insert your test script here
cat >test.sh <<'EOF'
a;b

a ; b # blank space

a	;	b	# tab

a
b

a;
b

a;b;
c

a;b;c;d;e

a
b
c
d
e

if a; then b; fi; if a; then b; fi

while a; do b; done; while a; do b; done

(a;b);(a;b) 

a|b;c|d
EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
    a \
  ;
    b
# 2
    a \
  ;
    b
# 3
    a \
  ;
    b
# 4
    a \
  ;
    b
# 5
  a
# 6
  b
# 7
    a \
  ;
    b
# 8
  c
# 9
    a \
  ;
    b \
  ;
    c \
  ;
    d \
  ;
    e
# 10
    a \
  ;
    b \
  ;
    c \
  ;
    d \
  ;
    e
# 11
    if
      a
    then
      b
    fi \
  ;
    if
      a
    then
      b
    fi
# 12
    while
      a
    do
      b
    done \
  ;
    while
      a
    do
      b
    done
# 13
    (
       a \
     ;
       b
    ) \
  ;
    (
       a \
     ;
       b
    )
# 14
      a \
    |
      b \
  ;
      c \
    |
      d
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
