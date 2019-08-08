#!/bin/bash

usage() {
    echo "usage: ./update.sh [--without-update]"
}

if [ "$1" = "--help" ] || [ "$1" = "-h" ] ; then
    usage
    exit 0
fi

if [ -f /tmp/ns3-last-update-time ]; then
    echo "Last update: " $(tail -1 /tmp/ns3-last-update-time)
else
    echo "Last update: null"
fi

if [ "$1" = "--without-update" ] || [ "$1" = "-w" ] ; then
    exit 0
fi

cp -rf scratch/* ~/ns-3.29/scratch
cp -rf src/* ~/ns-3.29/src

date +'%Y-%m-%d %H:%M:%S' >> /tmp/ns3-last-update-time