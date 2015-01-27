#! /bin/sh

#Bad cases for 1b

tmp=$0-$$.tmp
mkdir "$tmp" || exit
(
cd "$tmp" || exit
status=

# Sanity check, to make sure it works with at least one good example.
echo x >test0.sh || exit
../profsh -t test0.sh >test0.out 2>test0.err || exit
echo '# 1
  x' >test0.exp || exit
diff -u test0.exp test0.out || exit
test ! -s test0.err || {
  cat test0.err
  exit 1
}

n=1
for bad in \
  'if true; then echo missing fi;' \
  '((echo missing paren)' \
  'echo bad sequence;; echo bad!' \
  'while true; missing do; done' \
  'until true; do cat a; bad' \
  'echo hello>>>b'
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
