#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

NUM_FILES=101
FILE_SIZE=524288
CPU_LIMIT=30
NUM_ATTRIBUTES=8
RSA_KEY_LENGTH=2048

PUBLISH_DELAY_ENCRYPTION_WO_CPULIMIT=2
PUBLISH_DELAY_SIGNATURES_WO_CPULIMIT=1
PUBLISH_DELAY_BASE_WO_CPULIMIT=1

PUBLISH_DELAY_ENCRYPTION_W_CPULIMIT=6
PUBLISH_DELAY_SIGNATURES_W_CPULIMIT=6
PUBLISH_DELAY_BASE_W_CPULIMIT=6

PUBLISH_DELAY_ENCRYPTION_ANDROID=3
PUBLISH_DELAY_SIGNATURES_ANDROID=2
PUBLISH_DELAY_BASE_ANDROID=2

SECURITY_TESTER_LOG_DIR=log
LOG_DIR=microbenchmark

if [ -d "$LOG_DIR" ]
then
    rm -rf $LOG_DIR
fi

function exit_on_fail () {
    if [ $? -eq 0 ]
    then
        true
    else
        echo "failed! exiting"
        exit 1
    fi
}

mkdir -p $LOG_DIR

# Run core w/o cpulimit

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":100,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_BASE_WO_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_base
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":100,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_SIGNATURES_WO_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_signatures
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":100,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_WO_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_encryption_cached
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":100,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_WO_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_encryption_uncached
exit_on_fail

mv $SECURITY_TESTER_LOG_DIR/microbenchmark_base $LOG_DIR/microbenchmark_base_core_wo_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_signatures $LOG_DIR/microbenchmark_signatures_core_wo_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_cached $LOG_DIR/microbenchmark_encryption_cached_core_wo_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_uncached $LOG_DIR/microbenchmark_encryption_uncached_core_wo_cpulimit
exit_on_fail


# Run core w/ cpulimit

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_BASE_W_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_base
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_SIGNATURES_W_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_signatures
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_W_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_encryption_cached
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_W_CPULIMIT,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
sudo python security_tester.py --slevel FAILURE -c $CONFIG microbenchmark_encryption_uncached
exit_on_fail

mv $SECURITY_TESTER_LOG_DIR/microbenchmark_base $LOG_DIR/microbenchmark_base_core_w_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_signatures $LOG_DIR/microbenchmark_signatures_core_w_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_cached $LOG_DIR/microbenchmark_encryption_cached_core_w_cpulimit
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_uncached $LOG_DIR/microbenchmark_encryption_uncached_core_w_cpulimit
exit_on_fail

# Run android

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_BASE_ANDROID,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
python security_tester.py -e android --slevel FAILURE -c $CONFIG microbenchmark_base
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_SIGNATURES_ANDROID,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
python security_tester.py -e android --slevel FAILURE -c $CONFIG microbenchmark_signatures
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_ANDROID,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
python security_tester.py -e android --slevel FAILURE -c $CONFIG microbenchmark_encryption_cached
exit_on_fail

CONFIG="{\"microBenchmarkNumFiles\":${NUM_FILES},\"microBenchmarkFileSize\":$FILE_SIZE,\"microBenchmarkCPULimit\":$CPU_LIMIT,\"microBenchmarkPublishDelay\":$PUBLISH_DELAY_ENCRYPTION_ANDROID,\"microBenchmarkNumAttributes\":$NUM_ATTRIBUTES,\"microBenchmarkRSAKeyLength\":$RSA_KEY_LENGTH}"
python security_tester.py -e android --slevel FAILURE -c $CONFIG microbenchmark_encryption_uncached
exit_on_fail

mv $SECURITY_TESTER_LOG_DIR/microbenchmark_base $LOG_DIR/microbenchmark_base_android
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_signatures $LOG_DIR/microbenchmark_signatures_android
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_cached $LOG_DIR/microbenchmark_encryption_cached_android
exit_on_fail
mv $SECURITY_TESTER_LOG_DIR/microbenchmark_encryption_uncached $LOG_DIR/microbenchmark_encryption_uncached_android
exit_on_fail
