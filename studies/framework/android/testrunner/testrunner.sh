#!/bin/bash

#


# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

# TestRunner script to run tests created by TestGenerator in
# an automated fashion (using EMANE and CORE 4.3).
#
# Unfortunately some of this code is timing sensitive (where
# the timing is dependent on the hardware). So some adjustments
# may need to be made according to your system hardware.
#
# TestRunner optionally uses the cpulimit tool to bound the
# amount of cpu resources each lxc receives.
#
# The script expects a list of tests, and an output file to store
# statistics about the result of the tests.
#
# For example, you may have a directory Testsuite/ in the same dir
# as this file and a test case named:
#
# canary-N2D70s1-400x400-debug-off-prettygrid-pretty80211-pretty_app-2/ 1
#
# Then your test list file would contain the line:
#
# ./Testsuite/canary-N2D70s1-400x400-debug-off-prettygrid-pretty80211-pretty_app-2/ 1
#
# To disable a test in a test list file you can insert a "#" before the name
# and it will be skipped.
#
# You can specify a custom haggle executable (by default TestRunner uses the
# one in your PATH) using the "-c" option.
#
# The "-d" option will pause testrunner at the end of each test (prior to shutting
# down the haggle instances and CORE) to give the developer the opportunity
# to inspect the lxc and debug.

# Directory containing TestRunner
# CWD is cbmen-encoders/studies/STUDY/
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VERSION="0.5"
ENV_CHECK_SH="${DIR}/env_check.sh"
SYSINFO_SH="${DIR}/getsysinfo.sh"
NUM_TESTS="0"
MIN_MEM_MB="100"
NETWORK_TYPE="CORE"

print_usage() {
    echo "./testrunner.sh [-h] [-o] test_list test_output"
    exit 0
}

OPTIND=1
while getopts "h?o" opt; do
    case "$opt" in
        h|\?)
            print_usage
            exit 0
            ;;
        o)
            NETWORK_TYPE="OTA"
            ;;
    esac
done

shift $((OPTIND-1))

TEST_FILES="$1"   # test-list
OUTPUT_STATS="$2" # output.csv

# File used to echo testrunner STDOUT for email purposes
TMPFILE=$(tempfile)

# begin loading tests
if [ -r "${TEST_FILES}" ]; then
    echo "Loading tests from test file: ${TEST_FILES}" | tee ${TMPFILE}
    while read line
    do
        COMMENT=$(echo "$line" | grep "^#")
        if [ ! -z "${COMMENT}" ]; then
            continue
        fi
        # get the test name and the number of iterations
        TESTNAME=$(echo "$line" | awk '{print $1;}')
        TESTDIR=$(dirname ${TEST_FILES})
        NT=$(echo "$line" | awk '{print $2;}')
        if [ ! -d "${TESTDIR}/${TESTNAME}" ]; then
	    echo "${TESTDIR}/${TESTNAME}"
            echo "No such test: ${TESTNAME}" | tee ${TMPFILE}
            continue
        fi
        if [ -z ${NT} ]; then
            NT=1
        fi
        for (( i=1; i<=${NT}; i++ )); do
            TESTS="${TESTS} ${TESTDIR}/${TESTNAME}"
            NUM_TESTS=$((NUM_TESTS+1))
        done
    done < "${TEST_FILES}"
else
    echo "No test list found. Exiting."
    exit 1
fi

if [ ! -z "${OUTPUT_STATS}" ]; then
    echo "Saving stats to file: ${OUTPUT_STATS}" | tee ${TMPFILE}
fi

#
# Validate the system and find the tests:
#

echo "" | tee ${TMPFILE}
echo "Haggle Android Testrunner version ${VERSION}" | tee ${TMPFILE}
echo "---------------------------------------" | tee ${TMPFILE}
echo "- Environment check: " | tee ${TMPFILE}
if [ ! -x  ${ENV_CHECK_SH} ]; then
    echo "FAILED." | tee ${TMPFILE}
    echo "- Could not find environment check script, or bad permissions." | tee ${TMPFILE}
    echo "${ENV_CHECK_SH}" | tee ${TMPFILE}
    rm ${TMPFILE}
    exit 1
fi
bash ${ENV_CHECK_SH}
ENV_STATUS="$?"
if [ "${ENV_STATUS}" -ne "0" ]; then
    echo "Environment check: [FAILED]." | tee ${TMPFILE}
    rm ${TMPFILE}
    exit 1
fi
echo "    * Environment check: [PASSED]." | tee ${TMPFIL}
echo "---------------------------------------" | tee ${TMPFILE}
echo " - Found ${NUM_TESTS} tests." | tee ${TMPFILE}
if [ ${NUM_TESTS} -lt 1 ]; then
    echo "- Nothing to do..." | tee ${TMPFILE}
    rm ${TMPFILE}
    exit
fi

NUM_PASSED=0
NUM_FAILED=0

TOTAL_DURATION=0
for testi in ${TESTS}; do
    if [ ! -x "${testi}/echo_duration.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_duration.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    duration=$(bash "${testi}/echo_duration.sh")
    TOTAL_DURATION=$((TOTAL_DURATION + duration))
done
EST_END=$(date --date="${TOTAL_DURATION} seconds")
echo "    * Estimated duration (lower bound): ${TOTAL_DURATION} seconds, ${EST_END}" | tee ${TMPFILE}

EST_END_DATE=$(date --date="${TOTAL_DURATION} seconds" '+%s')
START_DATE=$(date '+%s')

EST_DURATION=$((EST_END_DATE-START_DATE))

#
# Begin the execution loop to run each test:
#

echo "- $(hostname) - starting test on: $(date)" | tee ${TMPFILE}
echo "---------------------------------------" | tee ${TMPFILE}

i=0
for testi in ${TESTS}; do

    i=$((i+1))

    # Restart ADB server
    adb kill-server
    sudo adb devices &> /dev/null

    $DIR/missingDevices.sh
    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Some ADB devices offline or missing." | tee ${TMPFILE}
        exit 1
    fi

    MEM_FREE_MB=$(free -m | awk '{print $3;}' | head -2 | tail -1)

    if [ ${MEM_FREE_MB} -lt "${MIN_MEM_MB}" ]; then
        echo "ERROR: System is dangerously low on memory, aborting." | tee ${TMPFILE}
        exit 1
    fi

    if [ ! -x "${testi}/echo_duration.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_duration.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    duration=$(bash "${testi}/echo_duration.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not get test duration." | tee ${TMPFILE}
        continue
    fi

    if [ ! -x "${testi}/echo_num_devices.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_num_devices.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    NUM_DEVICES=$(bash "${testi}/echo_num_devices.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not read device count." | tee ${TMPFILE}
        continue
    fi

    ANDROID_TEST_DIR="/data/tmp"

    bash $DIR/pre_test_cleanup.sh $NUM_DEVICES $ANDROID_TEST_DIR $NETWORK_TYPE

    if [ "$?" -ne "0" ]; then
        echo "ERROR: could not complete pre-test cleanup."
        exit 1
    fi

    if [ ! -x "${testi}/echo_testscenario.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_scenario.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    PRETTY_NOW=$(date '+%H:%M:%S')
    NOW=$(date '+%s')
    ELAPSED=$((NOW - START_DATE))
    PERDONE=$(awk 'BEGIN { printf "%.2f", 100*'${ELAPSED}'/'${EST_DURATION}' }')
    echo -ne "[${PRETTY_NOW}] ${PERDONE}%, Test $i/${NUM_TESTS}, ${duration}s, \"$(basename ${testi})\",..." | tee ${TMPFILE}

    if [ ! -x "${testi}/echo_output_path.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_output_path.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    outputfile=$(bash "${testi}/echo_output_path.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not get output path location." | tee ${TMPFILE}
        continue
    fi

    logpath="${outputfile}_logs$(date '+%s')"

    # some cleanup prior to running the tests
    rm -rf ${logpath} &> /dev/null

    if [ ! -x "${testi}/start_up.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no start_up.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    # Generate temporary files and edit core imn file to point to correct scenario
    bash "${testi}/start_up.sh" $NUM_DEVICES

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not execute test start up." | tee ${TMPFILE}
        continue
    fi

    testfile=$(bash "${testi}/echo_testscenario.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not get test location." | tee ${TMPFILE}
        continue
    fi

    if [ ! -r ${testfile} ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not read test scenario." | tee ${TMPFILE}
        continue
    fi

    if [ ! -x "${testi}/tear_down.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no tear_down.sh, or not executable)." | tee ${TMPFILE}
        continue
    fi

    START_TIME=$(date '+%s')

    if [ $NETWORK_TYPE == "CORE" ]; then
        core-gui -b "${testfile}" &> /dev/null &
    fi

    # Start Haggle on phones
    bash "${testi}start_haggle.sh" ${testi} $duration $NUM_DEVICES $ANDROID_TEST_DIR

    END_TIME=$(date '+%s')

    bash "${testi}/tear_down.sh"

    if [ "$?" -ne "0" ]; then
        echo "WARNING." | tee ${TMPFILE}
        echo "Test tear down failed, results may be invalid." | tee ${TMPFILE}
    fi

    # backup log files
    bash "$DIR/pull_results.sh" ${logpath} $NUM_DEVICES $ANDROID_TEST_DIR

    if [ -x  ${SYSINFO_SH} ]; then
        sudo bash ${SYSINFO_SH} > "${logpath}/sysinfo.log"
    fi

    if [ ! -z "${OUTPUT_STATS}" ]; then
        if [ -x "${testi}/echo_stats.sh" ]; then
            bash "${testi}/echo_stats.sh" "${logpath}/apps_output" $NUM_DEVICES "${logpath}/fail.log" $duration >> "${logpath}/stats.log"
            cat "${logpath}/stats.log" >> ${OUTPUT_STATS}
        fi
    fi

    echo $START_TIME >> "${logpath}/time.log"
    echo $END_TIME >> "${logpath}/time.log"

    if [ $NETWORK_TYPE == "CORE" ]; then
        sessid=$(ps aux | grep pycore | awk '{print $16;}' | sed  's/.*pycore\.\(.*\)\/.*/\1/g' | uniq)
        core-gui --closebatch ${sessid} &> /dev/null

        if [ "$?" -ne "0" ]; then
          echo "INVALID."
          echo "Could not stop core batch job."
          continue
        fi

        sudo bash -c "/etc/init.d/core-daemon stop &> /dev/null"

        # give core a few seconds to shutdown
        sleep 3

        # politely kill
        corepids=$(ps aux | egrep "(core.tcl|pycore|python.*core)" | awk '{print $2;}')
        for corepid in ${corepids}; do
            # suppress the "Terminated" messages
            disown $corepid &> /dev/null
            sudo bash -c "kill ${corepid} &> /dev/null"
        done

        sleep 1

        # rudely kill
        corepids=$(ps aux | egrep "(core.tcl|pycore|python.*core)" | awk '{print $2;}')
        for corepid in ${corepids}; do
            sudo bash -c "kill -9 ${corepid} &> /dev/null"
        done

        # more cleanup
        sudo rm -rf /tmp/pycore* &> /dev/null
    fi

    if [ "$?" -ne "0" ]; then
        continue
    fi

    RET="0"

    if [ ! -x "${testi}/validate.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "No validate script, or not executable." | tee ${TMPFILE}
    else
        #export logpath
        bash "${testi}/validate.sh" "${logpath}/apps_output" $NUM_DEVICES "${logpath}/fail.log"
        RET="$?"
    fi

    if [ "${RET}" -ne "0" ]; then
        # pull_failures.sh
        STATUS="FAILED."
        echo " [FAILED]" | tee ${TMPFILE}
        NUM_FAILED=$((NUM_FAILED + 1))
        echo "Errors:" | tee ${TMPFILE}
        cat "${logpath}/fail.log"
    else
        STATUS="PASSED."
        echo " [PASSED]" | tee ${TMPFILE}
        NUM_PASSED=$((NUM_PASSED + 1))
    fi
    echo "  * Log files saved: ${logpath}" | tee ${TMPFILE}

    if [ -x "${testi}/generate_gexf.py" ]; then
        python ${testi}/generate_gexf.py ${logpath}
    fi

    # backup testrunner for repeatablity
    pushd . &> /dev/null
    TMPDIR=$(mktemp -d)
    cd ${TMPDIR}
    mkdir testrunner
    cp ${DIR}/*.sh testrunner/
    mkdir testcase
    cp -rf ${testi} testcase/
    cp -f "${testi}/test.json" $logpath/ &> /dev/null
    tar -czf "${logpath}/testcase_testrunner.tar.gz" .
    rm -rf ${TMPDIR}
    popd &> /dev/null
done

END_DATE=$(date '+%s')

DURATION=$((END_DATE - START_DATE))

echo "---------------------------------------" | tee ${TMPFILE}
echo "- $(hostname) - done with test on: $(date)" | tee ${TMPFILE}
echo "- duration: ${DURATION} seconds" | tee ${TMPFILE}
echo "- ${NUM_TESTS} tests done." | tee ${TMPFILE}
echo "- ${NUM_PASSED} passed." | tee ${TMPFILE}
echo "- ${NUM_FAILED} failed." | tee ${TMPFILE}

rm ${TMPFILE}

if [ "${NUM_FAILED}" != "0" ]; then
    exit 1
fi

exit 0
