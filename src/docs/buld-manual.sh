#!/usr/bin/bash
#
# Build the manual
#
cd ${0%/*}

make latexpdf 
make html
