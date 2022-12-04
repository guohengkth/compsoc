#!/usr/bin/env bash 
# a wrapper to sudo readout.sh that check no other instances are running

TOOLS=${TOOLS:-./tools}

if [ ${USER} != "student" ] ; then
  echo 'this script must be run as student (not root)' 1>&2
  exit 1
fi
if [ `ps aux | grep channel_readall | grep -v grep | wc -l` != "0" ] ; then
  if [ "$1" == "-kill" ] ; then
    sudo killall channel_readall
    if [ `ps aux | grep channel_readall | grep -v grep | wc -l` != "0" ] ; then
      echo "couldn't kill other processes"
      exit 1
      fi
  else
    echo 'cannot start because the board is in use (another readout.sh is running)'
    echo 'do "./readout.sh -kill" if you want to kill the other instances'
    exit 1
  fi
fi

# check if monitoring tools have been made
[ -e $TOOLS/channel_init ] || (cd $TOOLS; make clean; make)

cd $TOOLS
sudo ./sudo-readout.sh
