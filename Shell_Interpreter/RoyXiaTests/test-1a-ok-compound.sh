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
if a;then b;fi

if a;then b;else c;fi

if a ; then b ; fi

if a ; then b ; else c ; fi

if a
then b
fi

if a
then b
else c
fi

if a<b>c; d<e>f; then a<b>c; d<e>f; else a<b>c; d<e>f; fi

if

a

then

b

fi

if

a

then

b

else

c

fi

if

a>b

c>d

then

a>b

c>d

fi

if

a>b

c>d

then

a>b

c>d

else

a>b

c>d

fi

while a;do b;done

until a;do b;done

while a ; do b ; done

while a
do b
done

while a<b>c; d<e>f; do a<b>c; d<e>f; done

while

a

do

b

done

while

a<b

c<d

do

a<b

c<d

done

if
  while a
  do b
  done
then
  while a
  do b
  done
else
  while a
  do b
  done
fi

while
  if a
  then b
  else c
  fi
do
  if a
  then b
  else c
  fi
done

if a;then b;fi<c

if a;then b;fi>c

if a;then b;fi<c>d

while a;do b;done<c

while a;do b;done>c

while a;do b;done<c>d

if a; then b; fi < c
if a; then b; fi > c

while a; do b; done < c
while a; do b; done > c

if a;
then b;
else c;
fi;

while a;
do b;
done;
EOF

# Insert your expected output here
cat >test.exp <<'EOF'
# 1
  if
    a
  then
    b
  fi
# 2
  if
    a
  then
    b
  else
    c
  fi
# 3
  if
    a
  then
    b
  fi
# 4
  if
    a
  then
    b
  else
    c
  fi
# 5
  if
    a
  then
    b
  fi
# 6
  if
    a
  then
    b
  else
    c
  fi
# 7
  if
      a<b>c \
    ;
      d<e>f
  then
      a<b>c \
    ;
      d<e>f
  else
      a<b>c \
    ;
      d<e>f
  fi
# 8
  if
    a
  then
    b
  fi
# 9
  if
    a
  then
    b
  else
    c
  fi
# 10
  if
      a>b \
    ;
      c>d
  then
      a>b \
    ;
      c>d
  fi
# 11
  if
      a>b \
    ;
      c>d
  then
      a>b \
    ;
      c>d
  else
      a>b \
    ;
      c>d
  fi
# 12
  while
    a
  do
    b
  done
# 13
  until
    a
  do
    b
  done
# 14
  while
    a
  do
    b
  done
# 15
  while
    a
  do
    b
  done
# 16
  while
      a<b>c \
    ;
      d<e>f
  do
      a<b>c \
    ;
      d<e>f
  done
# 17
  while
    a
  do
    b
  done
# 18
  while
      a<b \
    ;
      c<d
  do
      a<b \
    ;
      c<d
  done
# 19
  if
    while
      a
    do
      b
    done
  then
    while
      a
    do
      b
    done
  else
    while
      a
    do
      b
    done
  fi
# 20
  while
    if
      a
    then
      b
    else
      c
    fi
  do
    if
      a
    then
      b
    else
      c
    fi
  done
# 21
  if
    a
  then
    b
  fi<c
# 22
  if
    a
  then
    b
  fi>c
# 23
  if
    a
  then
    b
  fi<c>d
# 24
  while
    a
  do
    b
  done<c
# 25
  while
    a
  do
    b
  done>c
# 26
  while
    a
  do
    b
  done<c>d
# 27
    if
      a
    then
      b
    fi<c \
  ;
    if
      a
    then
      b
    fi>c
# 28
    while
      a
    do
      b
    done<c \
  ;
    while
      a
    do
      b
    done>c
# 29
  if
    a
  then
    b
  else
    c
  fi
# 30
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
