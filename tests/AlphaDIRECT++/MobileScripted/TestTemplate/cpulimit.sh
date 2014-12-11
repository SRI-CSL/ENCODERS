#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cpulimit --pid $1 --limit %%cpu_limit%% &

exit 0
