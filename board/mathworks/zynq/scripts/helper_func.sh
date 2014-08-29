#!/bin/bash

function print_msg() {
    CC_STARTMSG=$(tput setab 0)$(tput setaf 7)
    CC_NONE=$(tput sgr0)
    MSG=$1

    echo -e "${CC_STARTMSG}>>>   ${MSG}${CC_NONE}"
}

function print_err() {
    CC_STARTMSG=$(tput setab 1)$(tput setaf 7)$(tput bold)
    CC_NONE=$(tput sgr0)
    MSG=$1

    echo -e "${CC_STARTMSG}ERROR:   ${MSG}${CC_NONE}"
}
