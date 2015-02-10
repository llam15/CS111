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
(a)

(a; b)

(a
b)

(
a)

(a
)

(
a
)

( # blank space
a # blank space
) # blank space

(	# tab
a	# tab
)	# tab

(
a
b
)

(

a

b

)

(a|b)

(a<b>c)

(a;)

(if a;then b;else c;fi)

(if a;then b;fi)

(while a;do b;done)

(a)<b

(a) < b

(a)>c

(a) > c

(a)<b>c

(a) < b > c

( a ; ( if b; then c; ( d | e); fi)) <f>g
EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
  (
   a
  )
# 2
  (
     a \
   ;
     b
  )
# 3
  (
     a \
   ;
     b
  )
# 4
  (
   a
  )
# 5
  (
   a
  )
# 6
  (
   a
  )
# 7
  (
   a
  )
# 8
  (
   a
  )
# 9
  (
     a \
   ;
     b
  )
# 10
  (
     a \
   ;
     b
  )
# 11
  (
     a \
   |
     b
  )
# 12
  (
   a<b>c
  )
# 13
  (
   a
  )
# 14
  (
   if
     a
   then
     b
   else
     c
   fi
  )
# 15
  (
   if
     a
   then
     b
   fi
  )
# 16
  (
   while
     a
   do
     b
   done
  )
# 17
  (
   a
  )<b
# 18
  (
   a
  )<b
# 19
  (
   a
  )>c
# 20
  (
   a
  )>c
# 21
  (
   a
  )<b>c
# 22
  (
   a
  )<b>c
# 23
  (
     a \
   ;
     (
      if
        b
      then
          c \
        ;
          (
             d \
           |
             e
          )
      fi
     )
  )<f>g
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
