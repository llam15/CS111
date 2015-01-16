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
#this is a valid comment
######
   # ## #
	# fjkdsa;lf
a # don't mind me
b	# i am a comment and that is a tab
# i am also a comment
c # hi

a;b # pls don't screw up comment

a|b # pls

if a # halp
then b # be happy
else c # debug
fi # swagger

while a # ok
do b # good
done # excellent
EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
    a \
  ;
    b
# 2
  c
# 3
    a \
  ;
    b
# 4
    a \
  |
    b
# 5
  if
    a
  then
    b
  else
    c
  fi
# 6
  while
    a
  do
    b
  done
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
