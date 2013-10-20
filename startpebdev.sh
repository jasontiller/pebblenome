#!/bin/sh

# Modify this for your environment.
PEB_ROOT=~/pebble-dev

PEB_SDK=$PEB_ROOT/PebbleSDK-1.12/Pebble/sdk

# These links have to exist.
if [ ! -e include ]; then
    ln -s $PEB_SDK/include include
fi

if [ ! -e lib ]; then
    ln -s $PEB_SDK/lib lib
fi

if [ ! -e pebble_app.ld ]; then
    ln -s $PEB_SDK/pebble_app.ld pebble_app.ld
fi

if [ ! -e tools ]; then
    ln -s $PEB_SDK/tools tools
fi

if [ ! -e waf ]; then
    ln -s $PEB_SDK/waf waf
fi

if [ ! -e wscript ]; then
    ln -s $PEB_SDK/wscript wscript
fi

PATH=$PEB_ROOT/arm-cs-tools/bin:$PATH bash
