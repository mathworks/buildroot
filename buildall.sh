#!/bin/bash

SESSION="buildroot"
NO_DL=1
NO_CLEAN=0

usage() {
	cat >&2 <<EOL

Usage: buildall.sh [options] dir1 <dir2> <dir3> ...
Build all board targets in each directory
parameters:
	<dirN> 			    path to the board directory containing a build.sh script
options:
	--dl          	    Download sources beforehand
	--noclean	        Do not rebuild the board, just execute make in the output dir
EOL
	exit 1
}

OPTPARSE=`getopt -o h -l help,noclean,dl -n 'buildall.sh' -- "$@"`

strip_quote() {
    echo $1 | sed -e "s/^'\|'$//g"
}

parse_cmds() {
    while true; do
	    CMD=$1
	    ARGC=$#
	    case "${CMD}" in
	    --help|-h)
		    usage
		    ;;
        --dl)
		    shift		
		    NO_DL=0
		    ((ARGC=ARGC-1))
		    ;;
	    --noclean)
		    shift		
		    NO_CLEAN=1
		    ((ARGC=ARGC-1))
		    ;;
        -- ) 
            shift
		    ((ARGC=ARGC-1))
            break;
            ;;
	    *)
            break
		    ;;
	    esac
    done
	
    while [ $ARGC -gt 0 ]; do
        DIRS="$DIRS $(strip_quote $1)"
        shift
	    ARGC=$((ARGC-1))
    done
}

# Parse the input options / commands
parse_cmds $OPTPARSE

echo $NO_DL

for dir in ${DIRS}; do
    source ${dir}/build.sh
    BOARDS=${BOARD_LIST}
    arr=($BOARD_LIST)    
    BRD=${arr[0]}
    arr=($OS_LIST)    
    OS=${arr[0]}
    arr=($TC_LIST)    
    TC=${arr[0]}
    bld=${BRD}_${OS}_${TC}
    if [ ${NO_DL} == 0 ]; then
        ./build.sh -c -t "source" ${dir} $bld
    fi
    for brd in $BOARD_LIST; do
        bld=${brd}_${OS}_${TC}
        echo "$bld"
        if [ ${NO_CLEAN} == 0 ]; then
            xterm -hold -T $bld -e "./build.sh ${dir} $bld" &
            sleep 10
        else
            xterm -hold -T $bld -e "cd output/${bld} && make" &
        fi
    done
done



