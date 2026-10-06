#!/bin/sh
# Builds test tree

set -e
rm -rf testarea
mkdir testarea
cd testarea

printf 'This is the target\n' > target
cp target candidate1
ln target hardlink1
mkdir subdir
ln target subdir/hardlink2
cp target samesize_diff
printf 'X' | dd of=samesize_diff bs=1 seek=44 count=1 conv=notrunc status=none
printf 'short\n' > wrongsize
cp target binary_samesize
printf '\0' | dd of=binary_samesize bs=1 seek=10 count=1 conv=notrunc status=none
ln -s "$(pwd)/target" symlink_to_target
ln -s "$(pwd)/candidate1" symlink_to_duplicate
ln -s "$(pwd)/does_not_exist" dangling_symlink
ln -s loop_b loop_a
ln -s loop_a loop_b
mkdir noperm
echo secret > noperm/hidden
chmod 000 noperm
mkdir -p deep/nested/path
cp target deep/nested/path/candidate2
cd ..
echo "Test tree built under ./testarea"
echo "Restore permissions before deleting: chmod 755 testarea/noperm"
