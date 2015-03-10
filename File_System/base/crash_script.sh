#!/bin/sh

# These should work correctly

./crash_testing 5
echo epe > test/epe.txt
echo yas > test/yas.txt
ln test/epe.txt test/opo
ln test/opo test/ope
ln -s test/ope test/epo

#These will silently crash
echo epe >> test/epe.txt
echo noooo >> test/yas.txt
ln test/epe.txt test/ope
rm test/yas.txt

#Restore OSPFS to test
./crash_testing -1

# prints epe (instead of epe epe)
cat test/epe.txt

# yas still exists. Was not removed.
# prints yas (instead of yas noooo)
cat test/yas.txt

# ope does not exist
ln -s test/ope test/epo