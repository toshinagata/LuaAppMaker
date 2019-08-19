#!/bin/sh
#  Usage: sh path/to/record_build_date.sh [--with-svn-status] >build_dir/buildinfo.c
LAST_BUILD=`date '+%Y-%m-%d %H:%M:%S %Z'`
echo "char *gLastBuildString = \"$LAST_BUILD\";"
