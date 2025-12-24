#!/bin/bash
# $1 -> file path
# $2 -> address
# $3 -> size in byte

set -e

BASEDIR=$(dirname "$(readlink -f $0)")
SWD_SPD=$(cat "$BASEDIR/../swd_speed.txt")

tee TempFlashScript.jlink > /dev/null << EOT
usb $JLINK_SN
device OneKeyH7
SelectInterface swd
speed $SWD_SPD
RSetType 0
halt
SaveBin $1 $2 $3
rx 100
g
exit
EOT

JLinkExe -nogui 1 -commanderscript TempFlashScript.jlink

rm TempFlashScript.jlink