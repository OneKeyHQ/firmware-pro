#!/bin/bash
# $1 -> address
# $2 -> size in byte

tee TempFlashScript.jlink > /dev/null << EOT
usb $JLINK_SN
device OneKeyH7
SelectInterface swd
speed 20000
RSetType 0
halt
Erase $1 $2
rx 100
g
exit
EOT

JLinkExe -nogui 1 -commanderscript TempFlashScript.jlink

rm TempFlashScript.jlink