#!/bin/bash

while true
do
	for i in {1..6}; do
		./haggletest pub image${i}.jpg name=batterylife
		i=$((i + 1))
	done
	sleep 30
done

