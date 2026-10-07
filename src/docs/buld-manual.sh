#!/usr/bin/bash
#
# Build the manual
#
cd ${0%/*}

make html
make latexpdf 
