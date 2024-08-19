#!/bin/bash
# $1 -> target (0=Normal, 1=Board, 2=Boot)

case $1 in
"0")
    export TARGET_FLAG="0x00000000"
    ;;
"1")
    export TARGET_FLAG="0x64616F62"
    ;;
"2")
    export TARGET_FLAG="0x746F6F62"
    ;;
*)
    echo "Invalid boot target $1"
    exit -1
    ;;
esac

tee TempFlashScript.jlink >/dev/null <<EOT
usb $JLINK_SN
device OneKeyH7
SelectInterface swd
speed 20000
RSetType 0
rx 100
WaitHalt 100
write4 0x3003FFFC $TARGET_FLAG
go
exit
EOT

JLinkExe -nogui 1 -commanderscript TempFlashScript.jlink

rm TempFlashScript.jlink
