#!/bin/sh
#
# Helper script for the start-only case
#

case "$1" in
  start)
    # Continue script
	;;
  stop)
    # No shutdown action
	exit 0
	;;
  restart|reload)
	"$0" stop
	"$0" start
    exit $?
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac
