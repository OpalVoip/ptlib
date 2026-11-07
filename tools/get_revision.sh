#!/bin/sh

SRC="$1"
TMP="${SRC}.tmp"
DST="$2"

COMMIT=`git show 2> /dev/null | sed -n "s/^commit //p"`
if [ -z "$COMMIT" ]; then
   echo "git commit cannot be found"
   exit 1
fi

sed "s/GIT_COMMIT.*/GIT_COMMIT    \"$COMMIT\"/" $SRC > $TMP
if diff -q $DST $TMP >/dev/null 2>&1; then
   rm $TMP
   echo "Revision file unchanged"
else
   mv -f $TMP $DST
   echo "Revision file updated to $COMMIT"
fi
