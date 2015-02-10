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
cat if <if >if

cat then <then >then

cat else <else >else

cat fi <fi >fi

cat while <while >while

cat until <until >until

cat do <do >do

cat done <done >done

catif

catthen

catelse

catfi

catwhile

catuntil

catdo

catdone

if a if then else fi while until do done
then b if then else fi while until do done
else c if then else fi while until do done
fi 

while a if then else fi while until do done
do b if then else fi while until do done
done

ifcat

thencat

elsecat

ficat

whilecat

untilcat

docat

donecat

EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
  cat if<if>if
# 2
  cat then<then>then
# 3
  cat else<else>else
# 4
  cat fi<fi>fi
# 5
  cat while<while>while
# 6
  cat until<until>until
# 7
  cat do<do>do
# 8
  cat done<done>done
# 9
  catif
# 10
  catthen
# 11
  catelse
# 12
  catfi
# 13
  catwhile
# 14
  catuntil
# 15
  catdo
# 16
  catdone
# 17
  if
    a if then else fi while until do done
  then
    b if then else fi while until do done
  else
    c if then else fi while until do done
  fi
# 18
  while
    a if then else fi while until do done
  do
    b if then else fi while until do done
  done
# 19
  ifcat
# 20
  thencat
# 21
  elsecat
# 22
  ficat
# 23
  whilecat
# 24
  untilcat
# 25
  docat
# 26
  donecat
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
