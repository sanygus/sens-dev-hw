#!/bin/sh
while true
do
	python3 "`dirname $0`/ardsens.py"
	sleep 60
done