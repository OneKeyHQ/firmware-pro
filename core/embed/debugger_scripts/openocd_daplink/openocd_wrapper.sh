#!/bin/env bash

BASEDIR=$(dirname "$(readlink -f $0)")

openocd -s $BASEDIR -f OneKeyH7.cfg -c "halt" "$@"