#!/bin/sh

cat *.[ch] | grep "RFC [0-9][0-9][0-9][0-9]\:" | sort -n | uniq
