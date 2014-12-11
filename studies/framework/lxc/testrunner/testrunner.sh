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

SKIP_CHECK=1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VERSION="0.5"
ENV_CHECK_SH="${DIR}/env_check.sh"
SYSINFO_SH="${DIR}/getsysinfo.sh"
NUM_TESTS="0"
MIN_MEM_MB="100"
CORE_DAEMON=""
CORE_GUI=""

print_usage() {
    echo "./testrunner.sh [-c custom_haggle_path] [-h] [-d] test_list test_output"
    exit 0
}

HAS_CUST_HAGGLE=0
CUST_HAGGLE="/usr/local/bin/haggle"
OPTIND=1
DEBUG=0
while getopts "h?c:d" opt; do
    case "$opt" in
        h|\?)
            print_usage
            exit 0
            ;;
        c)
            CUST_HAGGLE=$OPTARG
            HAS_CUST_HAGGLE=1
            ;;
        d)
            DEBUG=1
            ;;
    esac
done

shift $((OPTIND-1))

TEST_FILES="$1"
OUTPUT_STATS="$2"

if [ "${HAS_CUST_HAGGLE}" == "1" ]; then
    if [ ! -x ${CUST_HAGGLE} ]; then
        echo "Cannot execute haggle: ${CUST_HAGGLE}"
        exit 0
    fi

    HAGGLE_BK="/usr/local/bin/haggle.bk.$(date '+%s')"
    sudo mv /usr/local/bin/haggle ${HAGGLE_BK}
    sudo ln -s ${CUST_HAGGLE} /usr/local/bin/haggle
fi

TMPFILE=$(tempfile)

# how long to wait before calling cpulimit on haggle
# processes:
CPULIMIT_PAUSE=20

# Auto email can be activated with ssmtp and setting the env
# variables (replacing 'sam@suns-tech.com' with your email):
# export TESTRUNNER_EMAIL_ENABLE=1
# export TESTRUNNER_EMAIL_FROM="sam@suns-tech.com"
# export TESTRUNNER_EMAIL_TO="sam@suns-tech.com"
TESTRUNNER_EMAIL_SUBJECT="AUTO: test_runner.sh auto runner results $(date)"
# Please see `email_guide.txt` for configuration information.

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

if which core-gui ; then
    CORE_DAEMON="core-daemon"
    CORE_GUI="core-gui"
    echo "Core 4.4+ detected." | tee ${TMPFILE}
elif which core ; then
    CORE_DAEMON="core"
    CORE_GUI="core"
    echo "Core 4.3 detected." | tee ${TMPFILE}
else
    echo "No CORE emulator found. Exiting."
    exit 1
fi

CORE_SERVICES_DIR="${HOME}/.core/myservices"
sudo sed -i "s|custom_services_dir = .*|custom_services_dir = ${CORE_SERVICES_DIR}|" /etc/core/core.conf

#
# Validate the system and find the tests:
#

echo "" | tee ${TMPFILE}
echo "Haggle CORE Testrunner version ${VERSION}" | tee ${TMPFILE}
echo "---------------------------------------" | tee ${TMPFILE}
if [ "${DEBUG}" != "0" ]; then
    echo "- DEBUG ENABLED."
fi
echo "- Environment check: " | tee ${TMPFILE}
echo "" | tee ${TMPFILE}
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
if [ "${SKIP_CHECK}" != "1" ]; then
    while true; do
        echo "Running this will delete \"${CORE_SERVICES_DIR}\"."
        echo "Files in /tmp may also be deleted."
        echo "Files in /cores may also be deleted."
        echo "It will also kill all proceses with the name 'core'."
        echo "Make sure everything important is backed up."
        read -p "Would you like to continue? [y/n]" yn
        case ${yn} in
            [Yy]* ) echo "Y"; break;;
            [Nn]* ) rm ${TMPFILE}; exit;;
            * ) echo "Please answer yes or no.";;
        esac
    done
fi

NUM_PASSED=0
NUM_FAILED=0

TOTAL_DURATION=0
# setting proper core dumps
for testi in ${TESTS}; do

    if [ ! -x "${testi}/echo_duration.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_duration.sh)." | tee ${TMPFILE}
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

    MEM_FREE_MB=$(free -m | awk '{print $3;}' | head -2 | tail -1)

    if [ ${MEM_FREE_MB} -lt "${MIN_MEM_MB}" ]; then
        echo "ERROR: System is dangerously low on memory, aborting." | tee ${TMPFILE}
        exit 1
    fi

    sudo rm -rf /cores

    sudo mkdir -p /cores
    sudo chmod a+rwx /cores
    sudo sh -c "echo /cores/core.%e.%p.%h.%t > /proc/sys/kernel/core_pattern"

    sudo bash -c "/etc/init.d/${CORE_DAEMON} stop &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: Could not stop ${CORE_DAEMON}." | tee ${TMPFILE}
        exit 1
    fi

    corepids=$(ps aux | egrep "(core.tcl|pycore|python.*core)" | awk '{print $2;}')
    for corepid in ${corepids}; do
        sudo bash -c "kill -9 ${corepid} &> /dev/null"
    done

    sudo bash -c "/etc/init.d/${CORE_DAEMON} start &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: could not start ${CORE_DAEMON}." | tee ${TMPFILE}
        exit 1
    fi

    service_files=$(ls ${testi}/CoreService/*.py 2> /dev/null)
    num_service_files=$(ls ${testi}/CoreService/*.py 2> /dev/null | wc -l)
    if [ "${num_service_files}" -lt "2" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no core services)." | tee ${TMPFILE}
        continue
    fi

    rm -f ${CORE_SERVICES_DIR}/*.{py,pyc,sh}

    cp ${service_files} ${CORE_SERVICES_DIR}

    # generate shell script to let python know of test location
    echo '#!/bin/bash'       >> "${CORE_SERVICES_DIR}/echo_test_dir.sh"
    echo "echo \"${testi}\"" >> "${CORE_SERVICES_DIR}/echo_test_dir.sh"
    chmod +x "${CORE_SERVICES_DIR}/echo_test_dir.sh"

    echo '#!/bin/bash'       >> "${CORE_SERVICES_DIR}/echo_user.sh"
    echo "echo \"${USER}\""  >> "${CORE_SERVICES_DIR}/echo_user.sh"
    chmod +x "${CORE_SERVICES_DIR}/echo_user.sh"

    if [ ! -x "${testi}/echo_duration.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_duration.sh)." | tee ${TMPFILE}
        continue
    fi

    duration=$(bash "${testi}/echo_duration.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not get test duration." | tee ${TMPFILE}
        continue
    fi

    PRETTY_NOW=$(date '+%H:%M:%S')
    NOW=$(date '+%s')
    ELAPSED=$((NOW - START_DATE))
    PERDONE=$(awk 'BEGIN { printf "%.2f", 100*'${ELAPSED}'/'${EST_DURATION}' }')
    echo -ne "[${PRETTY_NOW}] ${PERDONE}%, Test $i/${NUM_TESTS}, ${duration}s, \"$(basename ${testi})\",..." | tee ${TMPFILE}

    if [ ! -x "${testi}/echo_testscenario.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_scenario.sh)." | tee ${TMPFILE}
        continue
    fi

    if [ ! -x "${testi}/echo_output_path.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no echo_output_path.sh)." | tee ${TMPFILE}
        continue
    fi

    outputfile=$(bash "${testi}/echo_output_path.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not get output path location." | tee ${TMPFILE}
        continue
    fi

    logpath="${outputfile}_logs$(date '+%s')"

    if [ ! -x "${testi}/echo_fail_file.sh" ]; then
        echo "WARNING: old test without fail file!." | tee ${TMPFILE}
        failfile="/tmp/failfile"
    else
        failfile=$(bash "${testi}/echo_fail_file.sh")
    fi

    # some cleanup prior to running the tests
    rm -rf ${logpath} &> /dev/null
    sudo rm -rf /tmp/pycore* &> /dev/null
    rm -rf ${outputfile}.n* &> /dev/null
    rm -f ${failfile} &> /dev/null

    # need this or app.sh can't produce any output
    mkdir -p ${logpath}

    if [ ! -x "${testi}/start_up.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Test file is not specified correctly (no start_up.sh)." | tee ${TMPFILE}
        continue
    fi

    bash "${testi}/start_up.sh"

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
        echo "Test file is not specified correctly (no tear_down.sh)." | tee ${TMPFILE}
        continue
    fi

    ${CORE_GUI} -s "${testfile}" &> /dev/null &

    if [ "${DEBUG}" != "0" ]; then
        # used in stop_haggle.sh  to prevent instances from shutting down
        touch /tmp/debug_lock
    fi

    # cpulimit backwards compatability w/ existing tests
    if [ ! -x "${testi}/cpulimit_pid.sh" ]; then
        sleep ${duration}
    else
        sleep ${CPULIMIT_PAUSE}
        ps h -C haggle | grep haggle | awk '{print $1;}' | sudo xargs -n 1 ${testi}/cpulimit_pid.sh
        sleep $((duration - CPULIMIT_PAUSE))
    fi

    if [ "${DEBUG}" != "0" ]; then
        while true; do
            echo ""
            echo "---DEBUG MODE---"
            read -p "Are you done debugging? Would you like to continue to end the test (will wait 30 seconds to send shutdown)? [y]" yn
            case ${yn} in
                [Yy]* ) echo "Y"; rm -f /tmp/debug_lock; sleep 30; break;;
                * ) echo "Please answer yes.";;
            esac
        done
    fi

    bash "${testi}/tear_down.sh"

    if [ "$?" -ne "0" ]; then
        echo "WARNING." | tee ${TMPFILE}
        echo "Test tear down failed, results may be invalid." | tee ${TMPFILE}
    fi

    # backup log files
    dbfiles=$(ls /tmp/pycore*/n*/home*/*.db 2> /dev/null)
    dumpfiles=$(ls /tmp/pycore*/n*/home*/*.dump 2> /dev/null)
    logfiles=$(ls /tmp/pycore*/n*/home*/*.{log,xml} 2> /dev/null)
    mkdir -p ${logpath}
    for lfile in ${logfiles}; do
        node=$(echo ${lfile} | sed 's/.*\(n.*\)\.conf.*/\1/g')
        logname="${node}.$(basename ${lfile})"
        cp ${lfile} "${logpath}/${logname}"
    done

    cat ${outputfile}.n* >> "${logpath}/apps_output"

    if [ -x  ${SYSINFO_SH} ]; then
        sudo bash ${SYSINFO_SH} > "${logpath}/sysinfo.log"
    fi

    sessid=$(ps aux | grep pycore | awk '{print $16;}' | sed  's/.*pycore\.\(.*\)\/.*/\1/g' | uniq)

    RET="0"

    # copy doid list
    # this should come before validate.sh - JM
    for lfile in ${dbfiles}; do
        node=$(echo ${lfile} | sed 's/.*\(n.*\)\.conf.*/\1/g')
        logname="${node}.$(basename ${lfile})"
        python ${testi}/ls_db.py ${lfile} > "${logpath}/${logname}.hash_sizes.log"
    done

    if [ ! -x "${testi}/validate.sh" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "No validate script." | tee ${TMPFILE}
    else
        #export logpath
        bash "${testi}/validate.sh" "${logpath}/apps_output"
        RET="$?"
    fi

    if [ ! -z "${OUTPUT_STATS}" ]; then
        if [ -x "${testi}/echo_stats.sh" ]; then
            if [ "$i" == "1" ]; then
                bash "${testi}/echo_stats.sh" "${logpath}/apps_output" 1 >> "${logpath}/stats.log"
            else
                bash "${testi}/echo_stats.sh" "${logpath}/apps_output" >> "${logpath}/stats.log"
            fi
            cat "${logpath}/stats.log" >> ${OUTPUT_STATS}
        fi
    fi



    #if [ "${RET}" -ne "0" ]; then
    if true; then
        # copy core dumps (if any)
        if [ "$(ls /cores/ | wc -l)" != "0" ]; then
            cd /
            cp /usr/local/bin/haggle cores/haggle
            if [ "$(which pigz | wc -l)" != "0" ]
            then
                tar -I pigz -cf "${logpath}/cores.tar.gz" cores/
            else
                tar -czf "${logpath}/cores.tar.gz" cores/
            fi
            cd -
        fi

        # copy db files
        mkdir -p "${logpath}/db/"
        for lfile in ${dbfiles}; do
            node=$(echo ${lfile} | sed 's/.*\(n.*\)\.conf.*/\1/g')
            logname="${node}.$(basename ${lfile})"
            cp ${lfile} "${logpath}/db/${logname}"
            #python ${testi}/ls_db.py "${logpath}/db/${logname}" > "${logpath}/db/${logname}.hash_sizes.log"
        done 
        for lfile in ${dumpfiles}; do
            node=$(echo ${lfile} | sed 's/.*\(n.*\)\.conf.*/\1/g')
            logname="${node}.$(basename ${lfile})"
            cp ${lfile} "${logpath}/db/${logname}"
        done

        cd ${logpath}
        if [ "$(which pigz | wc -l)" != "0" ]
        then
            tar -I pigz -cf "${logpath}/db.tar.gz" db/
        else
            tar -czf "${logpath}/db.tar.gz" db/
        fi
        cd -

        python ${DIR}/../logreport/testreport.py -g ${DIR}/../logreport/scripts/greplist.txt ${logpath} > ${logpath}/report.log
        rm -rf "${logpath}/db/"
    fi

    (${CORE_GUI} --closebatch ${sessid} > /dev/null) &

    if [ "$?" -ne "0" ]; then
        echo "INVALID." | tee ${TMPFILE}
        echo "Could not stop core batch job." | tee ${TMPFILE}
        continue
    fi

    sudo bash -c "/etc/init.d/${CORE_DAEMON} stop &> /dev/null"

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

    if [ "${RET}" -ne "0" ]; then
        STATUS="FAILED."
        echo " [FAILED]" | tee ${TMPFILE}
        NUM_FAILED=$((NUM_FAILED + 1))
        echo "Errors:" | tee ${TMPFILE}
        [ -r "${failfile}" ] && cat ${failfile} | tee "${logpath}/fail_log"
    else
        STATUS="PASSED."
        echo " [PASSED]" | tee ${TMPFILE}
        NUM_PASSED=$((NUM_PASSED + 1))
    fi
    echo "  * Log files saved: ${logpath}" | tee ${TMPFILE}

    if [ -x "${testi}/generate_gexf.py" ]; then
        python ${testi}/generate_gexf.py ${logpath}
    fi

    # sometimes core does not cleanly close its interfaces!
    badifaces=$(sudo ifconfig | grep "b\.[0-9]*\.[0-9]*" | awk '{print $1;}' | wc -l)
    if [ "${badifaces}" != "0" ]; then
        echo "WARNING: core has left over virtual interfaces!" | tee ${TMPFILE}
        sudo ifconfig | grep "b\.[0-9]*\.[0-9]*" | awk '{print $1;}' | sudo xargs -I{} ifconfig {} down
    fi

    # backup testrunner for repeatablity
    pushd . &> /dev/null
    TMPDIR=$(mktemp -d)
    cd ${TMPDIR}
    mkdir testrunner
    cp ${DIR}/*.sh testrunner/
    mkdir testcase
    cp -rf ${testi} testcase/
    tar -czf "${logpath}/testcase_testrunner.tar.gz" .
    rm -rf ${TMPDIR}
    popd &> /dev/null

    # more cleanup
    sudo rm -rf /tmp/pycore* &> /dev/null
    rm -rf ${outputfile}.n* &> /dev/null
    rm -f ${failfile} &> /dev/null

    # crypto bridge cleanup
    rm -f /tmp/ccb_server.log_* &> /dev/null
    rm -f /tmp/ccb_socket_* &> /dev/null
    rm -f /tmp/haggletmpsecdata.* &> /dev/null
    rm -f /tmp/haggletmpobserverdata.* &> /dev/null
done

END_DATE=$(date '+%s')

DURATION=$((END_DATE - START_DATE))

echo "---------------------------------------" | tee ${TMPFILE}
echo "- $(hostname) - done with test on: $(date)" | tee ${TMPFILE}
echo "- duration: ${DURATION} seconds" | tee ${TMPFILE}
echo "- ${NUM_TESTS} tests done." | tee ${TMPFILE}
echo "- ${NUM_PASSED} passed." | tee ${TMPFILE}
echo "- ${NUM_FAILED} failed." | tee ${TMPFILE}

# EMAIL results (if applicable)

if [[ ! -z ${TESTRUNNER_EMAIL_ENABLE} && (${TESTRUNNER_EMAIL_ENABLE} == "1") ]]; then
    SSMTP=$(which ssmtp)
    if [ "$?" != "0" ]; then
        echo "Email enabled but no ssmtp." | tee ${TMPFILE}
        rm ${TMPFILE}
        exit 1
    fi

    if [ -z ${TESTRUNNER_EMAIL_FROM} ]; then
        echo "TESTRUNNER_EMAIL_FROM var not set." | tee ${TMPFILE}
        rm ${TMPFILE}
        exit 1
    fi

    if [ -z ${TESTRUNNER_EMAIL_TO} ]; then
        echo "TESTRUNNER_EMAIL_FROM var not set." | tee ${TMPFILE}
        rm ${TMPFILE}
        exit 1
    fi

    EMAIL_MSG=$(tempfile)
    echo "To: ${TESTRUNNER_EMAIL_TO}" >> ${EMAIL_MSG}
    echo "From: ${TESTRUNNER_EMAIL_FROM}" >> ${EMAIL_MSG}
    echo "Subject: ${TESTRUNNER_EMAIL_SUBJECT} pass: ${NUM_PASSED}, fail: ${NUM_FAILED}" >> ${EMAIL_MSG}
    echo "" >>  ${EMAIL_MSG}
    cat ${TMPFILE} >> ${EMAIL_MSG}
    echo "" >> ${EMAIL_MSG}
    echo "OUTPUT:" >> ${EMAIL_MSG}
    echo "" >> ${EMAIL_MSG}
    if [ ! -z ${OUTPUT_STATS} ]; then
        cat ${OUTPUT_STATS} >> ${EMAIL_MSG}
    fi
    sudo ssmtp ${TESTRUNNER_EMAIL_TO} < ${EMAIL_MSG}
    rm ${EMAIL_MSG}
fi

rm ${TMPFILE}

if [ "${HAS_CUST_HAGGLE}" == "1" ]; then
    sudo mv ${HAGGLE_BK} /usr/local/bin/haggle
fi

if [ "${NUM_FAILED}" != "0" ]; then
    exit 1
fi

exit 0

