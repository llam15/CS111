#! /bin/sh

# UCLA CS 111 Lab 1 - Test that syntax errors are caught.

# Copyright 2012-2014 Paul Eggert.

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
status=

# Sanity check, to make sure it works with at least one good example.
echo echo x >test0.sh || exit
../profsh -p test_profile0.txt test0.sh >test0.out 2>test0.err || exit
echo 'x' >test0.exp || exit
diff -u test0.exp test0.out || exit
test ! -s test0.err || {
  cat test0.err
  exit 1
}

n=1
for bad in \
  'sleep 1'
do
  echo "$bad" >test$n.sh || exit
#  ../profsh -p test_profile$n.txt test$n.sh >test$n.out 2>test$n.err && {
#    echo >&2 "test$n: unexpectedly succeeded for: $bad"
#    status=1
#  }
#  test -s test$n.err || {
#    echo >&2 "test$n: no error message for: $bad"
#    status=1
#  }
  ../profsh -p test_profile$n.txt test$n.sh >test$n.out 2>test$n.err || echo >&2 "test$n: Unexepctedly failed."
  hellag=$(cat test_profile$n.txt | grep 'sleep 1' | sed -n 's/^[0-9.]* \([0-9.]*\).*/\1/p')
  maxval=$(echo 1.100000000)
  minval=$(echo 0.900000000)

  if [ $(echo "$maxval < $hellag" | bc) -eq 1 ] || [ $(echo "$minval > $hellag" | bc) -eq 1 ]
  then
      echo >&2 "test$n: Unexpected real time: $hellag"
      status=1
  fi
  
  
# ((test $maxval -gt $hellag) && (test $minval -lt $hellag)) || exit

  n=$((n+1))
done

exit $status
) || exit

rm -fr "$tmp"
