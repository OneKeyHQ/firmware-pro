#!/bin/bash
# $1 -> begin address
# $2 -> end address

tee TempFlashScript.jlink > /dev/null << EOT
usb $JLINK_SN
device OneKeyH7
SelectInterface swd
speed 20000
RSetType 0
Erase $1 $2 noreset
rx 100
g
exit
EOT

JLinkExe -nogui 1 -commanderscript TempFlashScript.jlink

rm TempFlashScript.jlink