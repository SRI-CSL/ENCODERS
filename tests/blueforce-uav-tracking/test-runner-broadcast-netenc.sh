#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VERSION="0.2"
ENV_CHECK_SH="${DIR}/env_check.sh"

CORE_SERVICES_DIR="${HOME}/.core/myservices"


SKIP_CHECK=1
echo ""
echo "Haggle CORE Testrunner version ${VERSION}" 
echo "---------------------------------------"
echo -ne "- Environment check... "
if [ ! -x  ${ENV_CHECK_SH} ]; then
    echo "FAILED."
    echo "- Could not find environment check script, or bad permissions."
    echo "${ENV_CHECK_SH}"
    exit
fi
bash ${ENV_CHECK_SH}
ENV_STATUS="$?"
if [ "${ENV_STATUS}" -ne "0" ]; then
    echo "FAILED."
    echo "- Environment check failed."
    exit
fi
echo "PASSED."
echo "- Found ${NUM_TESTS} tests." 
echo "- "
echo "DONE."
if [ -z ${SKIP_CHECK} ]; then
    while true; do
        echo "Running this will delete \"${CORE_SERVICES_DIR}\"." 
        echo "Files in /tmp may also be deleted."
        echo "It will also kill all proceses with the name 'core'."
        echo "Make sure everything important is backed up."
        read -p "Would you like to continue? [y/n]" yn
        case ${yn} in
            [Yy]* ) echo "Y"; break;;
            [Nn]* ) exit;;
            * ) echo "Please answer yes or no.";;
        esac 
    done
fi

NUM_PASSED=0
NUM_FAILED=0

cp haggle-bcast-netenc-cache /tmp/haggle
cp config-flood-direct-fragmentation-coding-udp-bcast-caching.xml /tmp/config.xml

START_DATE=$(date '+%s')
echo "- $(hostname) - starting test on: $(date)"
echo "---------------------------------------"

tests=( "hope" )
for testi in "${tests[@]}" ; do

    sudo bash -c "/etc/init.d/core stop &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: Could not stop core."
        exit 
    fi

    corepids="$(ps aux | grep core | awk '{print $2;}')"
    for corepid in ${corepids}; do
        sudo bash -c "kill -9 ${corepid} &> /dev/null"
    done

    sudo bash -c "/etc/init.d/core start &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: could not start core."
        exit
    fi

    echo -ne "- Running \"$(basename ${testi})\"..."

    service_files=$(ls ${testi}/CoreService/*.py 2> /dev/null) 
    num_service_files=$(ls ${testi}/CoreService/*.py 2> /dev/null | wc -l) 
    if [ "${num_service_files}" -lt "2" ]; then
        echo "INVALID."
        echo "Test file is not specified correctly (no core services)."
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
        echo "INVALID."
        echo "Test file is not specified correctly (no echo_duration.sh)."
        continue
    fi

    duration=$(bash "${testi}/echo_duration.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID."
        echo "Could not get test duration."
        continue 
    fi

    echo -ne " (${duration} seconds)..."

    if [ ! -x "${testi}/echo_testscenario.sh" ]; then
        echo "INVALID."
        echo "Test file is not specified correctly (no echo_scenario.sh)."
        continue
    fi 

    if [ ! -x "${testi}/echo_output_path.sh" ]; then
        echo "INVALID."
        echo "Test file is not specified correctly (no echo_output_path.sh)."
        continue
    fi 

    outputfile=$(bash "${testi}/echo_output_path.sh")

    if [ "$?" -ne "0" ]; then
        echo "INVALID."
        echo "Could not get output path location."
        continue 
    fi

    # some cleanup prior to running the tests
    sudo rm -rf "${outputfile}_logs"
    sudo rm -rf /tmp/pycore*
    sudo rm -f ${outputfile}


    if [ ! -x "${testi}/cpulimit.sh" ]; then
        echo "INVALID."
        echo "Test file is not specified correctly (no cpulimit.sh)."
        continue
    fi 

    core -s "${testi}/IED_patrol.imn" &> /dev/null & 

    sleep 10
    ps h -C haggle | grep haggle | awk '{print $1;}' | sudo xargs -n 1 ${testi}/cpulimit.sh
    sleep $((duration - 10))

    # backup log files
    logfiles=$(ls /tmp/pycore*/n*/home*/*.{log,db,xml} 2> /dev/null)
    mkdir -p "${outputfile}_logs"
    for lfile in ${logfiles}; do
        node=$(echo ${lfile} | sed 's/.*\(n.*\)\.conf.*/\1/g')
        logname="${node}.$(basename ${lfile})"
        cp ${lfile} "${outputfile}_logs/${logname}"
    done

    cp ${outputfile} "${outputfile}_logs/apps_output"

    sessid=$(ps aux | grep pycore | awk '{print $16;}' | sed  's/.*pycore\.\(.*\)\/.*/\1/g' | uniq)

    core --closebatch ${sessid} &> /dev/null

    if [ "$?" -ne "0" ]; then
        echo "INVALID."
        echo "Could not stop core batch job."
        continue
    fi

    sudo bash -c "/etc/init.d/core stop &> /dev/null"

    corepids="$(ps aux | grep core | awk '{print $2;}')"
    for corepid in ${corepids}; do
        sudo bash -c "kill -9 ${corepid} &> /dev/null"
    done

    if [ ! -x "${testi}/validate.sh" ]; then
        echo "INVALID."
        echo "No validate script."
        continue
    fi

    bash "${testi}/validate.sh"

    if [ "$?" -ne "0" ]; then
        echo "FAILED."
        NUM_FAILED=$((NUM_FAILED + 1)) 
        echo "- Log files saved: ${outputfile}_logs"
    else
        echo "PASSED."
        NUM_PASSED=$((NUM_PASSED + 1)) 
    fi 


    # more cleanup
    sudo rm -rf /tmp/pycore*
    sudo rm -f ${outputfile}
done

END_DATE=$(date '+%s')

DURATION=$((END_DATE - START_DATE))

echo "---------------------------------------"
echo "- $(hostname) - done with test on: $(date)"
echo "- duration: ${DURATION} seconds"
echo "- ${NUM_TESTS} tests done."
echo "- ${NUM_PASSED} passed."
echo "- ${NUM_FAILED} failed."

