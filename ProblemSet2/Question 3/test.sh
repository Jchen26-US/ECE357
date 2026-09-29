#!/bin/sh
# Build a test tree for the scavenger hunt assignment.
# Run this, then invoke your program as:
#   ./hunt testarea/target testarea

set -e
rm -rf testarea
mkdir testarea
cd testarea

# 1. the target file
printf 'The quick brown fox jumps over the lazy dog.\n' > target

# 2. exact duplicate content, distinct inode
cp target candidate1

# 3. hard links to the target (nlink goes up)
ln target hardlink1
mkdir subdir
ln target subdir/hardlink2

# 4. same size, different content (flip the last byte)
cp target samesize_diff
printf 'X' | dd of=samesize_diff bs=1 seek=44 count=1 conv=notrunc status=none

# 5. different size entirely (should be skipped by the size filter)
printf 'short\n' > wrongsize

# 6. binary file with an embedded NUL byte, same size as target
cp target binary_samesize
printf '\0' | dd of=binary_samesize bs=1 seek=10 count=1 conv=notrunc status=none

# 7. symlink to the target
ln -s "$(pwd)/target" symlink_to_target

# 8. symlink to a duplicate
ln -s "$(pwd)/candidate1" symlink_to_duplicate

# 9. dangling symlink
ln -s "$(pwd)/does_not_exist" dangling_symlink

# 10. symlink loop (your code shouldn't follow it, so no infinite recursion)
ln -s loop_b loop_a
ln -s loop_a loop_b

# 11. permission-denied directory
mkdir noperm
echo secret > noperm/hidden
chmod 000 noperm

# 12. nested directory, to confirm recursion works
mkdir -p deep/nested/path
cp target deep/nested/path/candidate2

cd ..
echo "Test tree built under ./testarea"
echo "Restore permissions before deleting: chmod 755 testarea/noperm"
