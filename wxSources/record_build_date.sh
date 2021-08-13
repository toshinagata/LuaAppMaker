#!/bin/sh
#  Usage: sh path/to/record_build_date.sh [--with-svn-status] >build_dir/buildinfo.c
LAST_BUILD=`date '+%Y-%m-%d %H:%M:%S %Z'`
LAST_TAG=`git tag --sort=-taggerdate | head -1`
echo "const char *gLastBuildString = \"$LAST_BUILD\";"
echo "const char *gVersionString = \"$LAST_TAG\";"
