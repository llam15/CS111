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

# Enter your test in here, e.g.
# '`' \
n=1
for bad in \
  '`' \
  'a#' \
  'a#f' \
  '$' \
  '&' \
  '*' \
  '=' \
  '\' \
  '"' \
  "'" \
  '?' \
  '~' \
  '{' \
  '}' \
  '[' \
  ']' \
  '||' \
  '>>' \
  '<<' \
  'a>>b' \
  'a<<b' \
  'a>b<c' \
  'a || b' \
  '  a  ||  b '
do
  echo "$bad" >test$n.sh || exit
  ../profsh -t test$n.sh >test$n.out 2>test$n.err && {
    echo >&2 "test$n: unexpectedly succeeded for: $bad"
    status=1
  }
  test -s test$n.err || {
    echo >&2 "test$n: no error message for: $bad"
    status=1
  }
  n=$((n+1))
done

exit $status
) || exit

rm -fr "$tmp"
