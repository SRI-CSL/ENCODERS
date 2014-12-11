#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


# TestRunner / TestGenerator self test script (for regression
# testing). 

SKIP_CHECK=1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} TestRunner/TestGenerator Self Test"
PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} This can take awhile... (> 20 minutes)"

pushd . &> /dev/null
cd $DIR 

if [ ! -z "$1" ]; then
    BRANCH_LIST="$1"        
else
    BRANCH_LIST="${DIR}/branch_list"
fi

if [ ! -r "${BRANCH_LIST}" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] Could not load branch list: ${BRANCH_LIST}"
    exit 0
fi

BRANCHES=""
BRANCH_COUNT=0

while read line
do
    COMMENT=$(echo "$line" | grep "^#")
    if [ ! -z "${COMMENT}" ]; then
        continue
    fi
    BRANCH_COUNT=$((BRANCH_COUNT+1))
    BRANCHNAME=$(echo "$line" | awk '{print $1;}')
    BRANCHES="${BRANCHES} ${BRANCHNAME}"
done < ${BRANCH_LIST}

if [ "${SKIP_CHECK}" != "1" ]; then
    while true; do
        echo "Running this will switch your git directory. Make sure everything is committed"
        read -p "Would you like to continue? [y/n]" yn
        case ${yn} in
            [Yy]* ) echo "Y"; break;;
            [Nn]* ) rm ${TMPFILE}; exit;;
            * ) echo "Please answer yes or no.";;
        esac 
    done
fi

OLD_BRANCH=$(git branch | grep "*" | sed "s/* //")
if [ "$?" != "0" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] Could not get branch name, this script expects to be in a git directory."
    exit 1
fi

if [ "${OLD_BRANCH}" != "evaluation" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] Not in 'evaluation' branch!"
    exit 1
fi

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} Self-testing on branches: ${BRANCHES}"

NUM_CHANGES=$(git diff | wc -l)
if [ "0" != "${NUM_CHANGES}" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] You have uncommited changes in this branch '${OLD_BRANCH}', please commit them and re-execute."
    exit 1
fi

git fetch origin &> /dev/null
if [ "$?" != "0" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW}: STATUS: [FAIL] Could not fetch origin."
    exit 1
fi

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} Pulling branch 'evaluation' from origin..."
git pull origin evaluation &> /dev/null
if [ "$?" != "0" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] Could not pull branch from 'origin/evaluation'"
    popd &> /dev/null
    exit 1
fi

for BRANCH in ${BRANCHES}; do
    pushd . &> /dev/null
    git checkout ${BRANCH} &> /dev/null
    if [ "$?" != "0" ]; then
    	PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} STATUS: [FAIL] Could not checkout branch (did you commit your changes?): ${BRANCH}"
        popd &> /dev/null
        exit 1
    fi

    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} Pulling branch ${BRANCH} from origin..."
    git pull origin ${BRANCH} &> /dev/null
    if [ "$?" != "0" ]; then
    	PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} STATUS: [FAIL] Could not pull branch from 'origin/${BRANCH}'"
        popd &> /dev/null
        #exit 1
    fi
    cd ../haggle/
    HASH=$(git rev-parse --short HEAD)
    if [ -f /usr/local/bin/haggle-${BRANCH}-${HASH} ]; then
        PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} binary for ${HASH} already compiled, skipping recompilation."
        popd &> /dev/null
        continue;
    fi
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} Compiling branch '${BRANCH}'..."
    ./autogen.sh &> /dev/null
    if [ "$?" != "0" ]; then
    	PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} STATUS: [FAIL] Autogen failed for branch '${BRANCH}'"
        exit 1
    fi
    ./configure &> /dev/null
    if [ "$?" != "0" ]; then
    	PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} STATUS: [FAIL] Configure failed for branch '${BRANCH}'"
        exit 1
    fi
    make -j5 &> /dev/null
    if [ "$?" != "0" ]; then
    	PRETTY_NOW="[$(date '+%H:%M:%S')]"
        echo "${PRETTY_NOW} STATUS: [FAIL] Could not compile branch '${BRANCH}'"
        popd &> /dev/null
        exit 1
    fi

    sudo cp bin/haggle /usr/local/bin/haggle-${BRANCH}-${HASH}
    sudo chmod a+x /usr/local/bin/haggle-${BRANCH}-${HASH}
    sudo touch /usr/local/bin/haggle-${BRANCH}-latest
    sudo rm /usr/local/bin/haggle-${BRANCH}-latest
    sudo ln -s  /usr/local/bin/haggle-${BRANCH}-${HASH} /usr/local/bin/haggle-${BRANCH}-latest
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} ln -s  /usr/local/bin/haggle-${BRANCH}-${HASH} /usr/local/bin/haggle-${BRANCH}-latest"
    popd &> /dev/null
done

git checkout ${OLD_BRANCH} &> /dev/null

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} Generating fresh tests."

rm -rf ./Testsuite/*
cd ./TestGenerator
python test_generator.py sample_test_spec.json &> /dev/null

if [ "$?" != "0" ]; then
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} STATUS: [FAIL] test_generator.py could not generate tests"
    exit
fi

cd ../

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} Running fresh tests (this can take awhile...)"

BRANCH_I=0
for BRANCH in ${BRANCHES}; do
    BRANCH_I=$((BRANCH_I + 1))
    PRETTY_NOW="[$(date '+%H:%M:%S')]"
    echo "${PRETTY_NOW} (${BRANCH_I}/${BRANCH_COUNT}) Running self test for branch: ${BRANCH}"
    REAL_PATH=$(readlink -f /usr/local/bin/haggle-${BRANCH}-latest)
    ./testrunner.sh -c ${REAL_PATH} test_list selftest_output.csv &> /dev/null
    if [ "$?" != "0" ]; then
        echo "${PRETTY_NOW} STATUS: [FAIL] test_runner had tests fail!"
        exit 1
    fi
done

rm -rf ./Testsuite/*

popd &> /dev/null

PRETTY_NOW="[$(date '+%H:%M:%S')]"
echo "${PRETTY_NOW} STATUS: [PASSED]"

exit 0
