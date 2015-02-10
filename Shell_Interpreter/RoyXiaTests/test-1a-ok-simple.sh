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
a

ab

abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!%+,-./:@^_

a b

a   b # blank spaces

a	b	# tabs

a<b

a>c

a<b>c

a b<c

a b>d

a b<c>d

a < b # blank space

a > c

a < b > c

a	<	b	# tab

a	>	c	

a	<	b	>	c	
EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
  a
# 2
  ab
# 3
  abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!%+,-./:@^_
# 4
  a b
# 5
  a b
# 6
  a b
# 7
  a<b
# 8
  a>c
# 9
  a<b>c
# 10
  a b<c
# 11
  a b>d
# 12
  a b<c>d
# 13
  a<b
# 14
  a>c
# 15
  a<b>c
# 16
  a<b
# 17
  a>c
# 18
  a<b>c
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
