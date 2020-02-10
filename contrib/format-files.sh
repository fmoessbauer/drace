#!/bin/bash
#
# DRace, a dynamic data race detector
# Copyright 2018 Siemens AG
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
# SPDX-License-Identifier: MIT

# set to anything, if we are in ci and
# a patch should be generated
cimode="$1"

files=$(find . -type f \
	! -path "*/build*" \
	! -path "*/submodules/*" \
	! -path "./vendor/*" \
	! -path "./ManagedResolver/include/prebuild/*" \
	\( -name "*.hpp" -o -name "*.h" -o -name "*.cpp" \))
for file in $files; do
	[ "$cimode" == "" ] && echo "Format $file"
	clang-format -i -style=file $file
done

#in non-ci mode exit here
[ "$cimode" == "" ] && exit 0;

format_diff=`git diff`
if [ "$format_diff" != "" ]; then
	echo "Source files are not properly formatted. Diff:"
	echo "$format_diff"
	exit 1
fi
