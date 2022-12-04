#!/usr/bin/env bash
# rebuilds what's necessary and then restarts the new applications three seconds after the compiling & loading all data to the RISC-V cores

if [ ${USER} != "student" ] ; then
  echo 'this script must be run as student (not root)' 1>&2
  exit 1
fi

# pass arguments to rerun
# WARNING: we've recompiled all sources, so you should only use the -c and -s* flags
make && ./rerun.sh $*
