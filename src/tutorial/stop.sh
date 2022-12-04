#!/usr/bin/env bash
# stop			-- stop all partitions on all tiles
# stop [tile ...]	-- stop all partitions on tiles

TOOLS=${TOOLDIR:-./tools}
DYNLOAD=${DYNLOAD:-$TOOLS/dynload-0.05}

if [ ${USER} != "student" ] ; then
  echo 'this script must be run as student (not root)' 1>&2
  exit 1
fi

if [ "$1" == "" ] ; then
  sudo $DYNLOAD 0 stop
  sudo $DYNLOAD 1 stop
  sudo $DYNLOAD 2 stop
else
  for i in $* ; do 
    sudo $DYNLOAD $i stop
  done
fi
