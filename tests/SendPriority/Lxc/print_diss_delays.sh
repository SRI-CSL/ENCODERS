#!/bin/bash

for d in $(ls r*/$1); do
    cat $d | grep "Received," | grep -v "1024$" | grep -v "5120$" | grep -v "10240$" | awk -F, '{printf "%d\n", $7}' | sort -n > $d.processed
done

for f in $(cat r*/$1.processed | sort -n | uniq); do
    A=""
    for d in $(ls r*/$1.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo $f $A
done
