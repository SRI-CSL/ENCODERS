#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cpulimit --pid $1 --limit 20.0 &

exit 0
