#!/usr/bin/env bash
# exec [seconds] [options for run.sh, which passes them on to rerun.sh]
# equivalent to ./run.sh; sleep SEC; ./stop.sh

if [ "$1" = "" ] ; then
  echo "warning: no sleep duration given, running for 1 second" 1>&2
  S=1
else
  S="$1"
  shift
fi
./run.sh $* && sleep $S && ./stop.sh
