#!/bin/bash

res=''

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

function get_cfg_var() {
	local var=$1
	res=`cat $BR2_CONFIG | grep ^$var | sed -e 's/BR2_.*=//' | tr -d '"'`
}
