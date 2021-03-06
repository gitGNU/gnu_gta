#!/usr/bin/env bash

# Copyright (C) 2010, 2011, 2013
# Martin Lambers <marlam@marlam.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

set -e

TMPD="`mktemp -d tmp-\`basename $0 .sh\`.XXXXXX`"

$GTA create -d 7,11,13 -c int8,int8,int8 -n 2 "$TMPD"/a.gta

$GTA info --help 2> "$TMPD"/help.txt

$GTA info "$TMPD"/a.gta "$TMPD"/a.gta "$TMPD"/a.gta 2> /dev/null
$GTA info -s "$TMPD"/a.gta 2> /dev/null

$GTA create -d 10 -n5 > "$TMPD"/empty0.gta
$GTA create -c uint8 -n5 > "$TMPD"/empty1.gta
$GTA info "$TMPD"/empty0.gta "$TMPD"/empty1.gta 2> /dev/null
$GTA info -s "$TMPD"/empty0.gta "$TMPD"/empty1.gta 2> /dev/null

rm -r "$TMPD"
