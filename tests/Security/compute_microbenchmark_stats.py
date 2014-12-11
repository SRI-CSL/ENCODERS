#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Microbenchmark Statistics Computer

Usage:
  compute_microbenchmark_stats.py

Options:
  -h --help                     Show this screen.
"""

import math
import os
import re
from collections import OrderedDict
from docopt import docopt

def createStatsDict(key, values, skip):
    computeWith = values[skip:]

    if len(computeWith) == 0:
        mean, dev, minimum, maximum, sum_ = (0, 0, 0, 0, 0)
    else:
        mean = sum(computeWith) / len(computeWith)
        dev = math.sqrt( sum((v-mean)**2 for v in computeWith) / len(computeWith))
        minimum = min(computeWith)
        maximum = max(computeWith)
        sum_ = sum(computeWith)

    retval = dict(
                mean = round(mean, 3),
                dev = round(dev, 3),
                minimum = round(minimum, 3),
                maximum = round(maximum, 3),
                skip = skip,
                sum = sum_
                )
    retval[key] = values
    return retval

def createTimesDict(times, skip):
    return createStatsDict('times', times, skip)

def createSizesDict(sizes):
    return createStatsDict('sizes', sizes, 0)

def computeEndtoEndTime(input, skip=1):
    times = []
    for line in input.readlines()[1:]:
        values = line.split(',')
        time = float(values[-3])
        times.append(time)

    return createTimesDict(times, skip)

def computeDOsToTrack(subs, logs):
    dos = []
    for line in subs.readlines()[1:]:
        values = line.split(',')
        do = values[1]
        dos.append(do)

    pts = dos[:]
    pattern = re.compile("""^[0-9]*\.[0-9]*\:\[[0-9]*\-[0-9\-]*\]{SecurityHelper::doTask}: Finished encrypting - new encrypted Data Object \[([a-f0-9]*)\] is replacing plaintext object \[([a-f0-9]*)\]""", re.MULTILINE)
    for log in logs:
        matches = pattern.findall(log.read())
        for (new, old) in matches:
            if old in dos:
                dos.append(new)

    cts = [do for do in dos if do not in pts]
    return (dos, pts, cts)

def computeNDsToTrack(logs):
    nds = []
    pattern = re.compile("""^[0-9]*\.[0-9]*\:\[[0-9]*-[0-9\-]*\]{NodeManager::onReceiveNodeDescription}: Node description \[(?P<nd>[a-f0-9]*)\] of node [A-Za-z0-9]* \[[a-f0-9]*\] received""", re.MULTILINE)
    for log in logs:
        matches = pattern.findall(log.read())
        for nd in matches:
            if nd not in nds:
                nds.append(nd)

    return nds

def computeSDRsToTrack(logs):
    sdrs = []
    pattern = re.compile("""^[0-9]*\.[0-9]*\:\[[0-9]*-[0-9\-]*\]{DataObject::print}: DataObject id=(?P<do>[a-f0-9]*):..<\?xml version="1\.0"\?>.<Haggle create_time="[0-9]*\.[0-9]*" persistent="yes">...<Attr name="(?P<type>SecurityDataRequest|SecurityDataResponse)" weight="1">(?P<value>[a-z0-9_]*)</Attr>""", re.MULTILINE | re.DOTALL)
    for log in logs:
        matches = pattern.findall(log.read())
        for match in matches:
            (sdr, type_, value) = match
            if sdr not in sdrs:
                sdrs.append(sdr)

    return sdrs

def computeSpecificTimes(inputs, dos):
    times = {'signDataObject': [], 'verifyDataObject': [], 'generateCapability': [], 'useCapability': [], 'encryptDataObject': [], 'decryptDataObject': []}
    pattern = re.compile("""^([0-9]*\.[0-9]*)\:\[[0-9]*\-[0-9]*\]{SecurityHelper::doTask}: ([a-zA-z]*) (Calling|Succeeded|Failed) for Data Object \[([a-f0-9]*)\]""", re.MULTILINE)

    for input in inputs:
        matches = pattern.findall(input.read())
        groupedMatches = zip(matches[0::2], matches[1::2])
        for (first, second) in groupedMatches:
            # print '%s -> %s' % (first, second)
            (time1, fn1, state1, do1) = first
            (time2, fn2, state2, do2) = second

            time1, time2 = float(time1), float(time2)

            if time2 < time2:
                raise Exception('time2 < time1! This is impossible! %s - %s' % (first, second))

            if fn1 != fn2:
                raise Exception('Grouped match returned different functions! This is impossible! %s - %s' % (first, second))

            if do1 != do2 and fn != 'useCapability':
                raise Exception('Grouped match returned different data objects! This is impossible! %s - %s' % (first, second))

            if state1 != 'Calling':
                raise Exception('state1 != Calling! This is impossible! %s - %s' % (first, second))

            if state2 not in ['Succeeded', 'Failed']:
                raise Exception('state2 not in [Succeeded, Failed]! This is impossible! %s - %s' % (first, second))

            if state2 == 'Failed' or (fn1 != 'signDataObject' and do2 not in dos):
                continue

            times[fn1].append(time2-time1)

    return {k : createTimesDict(v, 0) for (k, v) in times.items()}

def computeDOSizes(inputs, dos):
    sizes = []
    pattern = re.compile("""^[0-9]*\.[0-9]*\:\[(?P<thread>[0-9]*\-[0-9]*)\]{Protocol::receiveDataObject}: ProtocolTCPClient:(?P<proto>[0-9]*) Metadata header received \[BytesPut=(?P<bytesPut>[0-9]*) totBytesPut=(?P<totBytesPut>[0-9]*) totBytesRead=(?P<totBytesRead>[0-9]*) bytesRemaining=(?P<bytesRemaining>[0-9]*)\].[0-9]*\.[0-9]*\:\[(?P=thread)\]{Protocol::receiveDataObject}: ProtocolTCPClient:(?P=proto) Incoming data object \[(?P<do>[a-f0-9]*)\] from peer [a-z0-9]*""", re.MULTILINE | re.DOTALL)

    for input in inputs:
        matches = pattern.findall(input.read())
        for match in matches:
            (thread, proto, bytesPut, totBytesPut, totBytesRead, bytesRemaining, do) = match

            bytesPut, totBytesPut, totBytesRead, bytesRemaining = int(bytesPut), int(totBytesPut), int(totBytesRead), int(bytesRemaining)

            if do in dos:
                sizes.append(totBytesRead + bytesRemaining)

    # if len(sizes) != len(dos):
        # print "Couldn't match all DOs! %s - %s\n" % (sizes, dos)

    return createSizesDict(sizes)

def getBenchmarkStats(basedir, benchmark):
    if not os.path.exists(os.path.join(basedir, benchmark)):
        raise Exception('Can not find benchmark directory %s! Aborting' % benchmark)

    batchSubsFile = os.path.join(basedir, benchmark, 'ALICE_batchsubs_object1.log')
    if not os.path.exists(batchSubsFile):
        raise Exception('Can not find batch subs file %s! Aborting' % batchSubsFile)

    with open(batchSubsFile) as input:
        endToEndTimes = computeEndtoEndTime(input)

    # print endToEndTimes

    aliceLogFile = os.path.join(basedir, benchmark, 'ALICE_haggle.log')
    if not os.path.exists(aliceLogFile):
        raise Exception('Can not find ALICE log file %s! Aborting' % aliceLogFile)

    bobLogFile = os.path.join(basedir, benchmark, 'BOB_haggle.log')
    if not os.path.exists(bobLogFile):
        raise Exception('Can not find BOB log file %s! Aborting' % bobLogFile)

    with open(batchSubsFile) as subs:
        with open(aliceLogFile) as alog:
            with open(bobLogFile) as blog:
                (dos, pts, cts) = computeDOsToTrack(subs, [alog, blog])

    # print dos

    with open(aliceLogFile) as alog:
        with open(bobLogFile) as blog:
            specificTimes = computeSpecificTimes([alog, blog], dos)

    # print specificTimes

    if len(cts) == 0:
        sizedDOs = pts[:]
    else:
        sizedDOs = cts[:]

    with open(aliceLogFile) as alog:
        doSizes = computeDOSizes([alog], sizedDOs)

    # print doSizes

    with open(aliceLogFile) as alog:
        with open(bobLogFile) as blog:
            nds = computeNDsToTrack([alog, blog])

    # print nds

    with open(aliceLogFile) as alog:
        with open(bobLogFile) as blog:
            ndSizes = computeDOSizes([alog, blog], nds)

    # print ndSizes

    with open(aliceLogFile) as alog:
        with open(bobLogFile) as blog:
            sdrs = computeSDRsToTrack([alog, blog])

    # print sdrs

    with open(aliceLogFile) as alog:
        with open(bobLogFile) as blog:
            sdrSizes = computeDOSizes([alog, blog], sdrs)

    # print sdrSizes

    return dict(endToEndTimes=endToEndTimes, specificTimes=specificTimes, doSizes=doSizes, ndSizes=ndSizes, sdrSizes=sdrSizes)

def printSizes(title, sizes):
    print '    %20s : %.0f +- %.0f (%.0f -> %.0f) => %.0f' % (title, sizes['mean'], sizes['dev'], sizes['minimum'], sizes['maximum'], sizes['sum'])

def printTimes(title, times):
    print '    %20s : %.3f +- %.3f (%.3f -> %.3f)' % (title, times['mean'], times['dev'], times['minimum'], times['maximum'])

def printTestResult(title, result):
    print '%s: ' % title
    printTimes('endToEnd', result['endToEndTimes'])
    printTimes('signDataObject', result['specificTimes']['signDataObject'])
    printTimes('verifyDataObject', result['specificTimes']['verifyDataObject'])
    printTimes('generateCapability', result['specificTimes']['generateCapability'])
    printTimes('useCapability', result['specificTimes']['useCapability'])
    printTimes('encryptDataObject', result['specificTimes']['encryptDataObject'])
    printTimes('decryptDataObject', result['specificTimes']['decryptDataObject'])
    printSizes('doSizes', result['doSizes'])
    printSizes('ndSizes', result['ndSizes'])
    printSizes('sdrSizes', result['sdrSizes'])

def printSummaryTable(rows):
    lens = [map(len, l) for l in rows]
    colLengths = map(max, zip(*lens))

    fmtblk = '| %%-%ss'
    fmt = ' '.join(fmtblk % l for l in colLengths) + ' |'

    sepLen = 3 * len(colLengths) + sum(colLengths) + 1
    sep = '-' * sepLen

    print sep
    for row in rows:
        print fmt % tuple(row)
        print sep

def printOperationTimeSummaryTable(title, results, baseBenchmarks, crossConfigs):
    operations = ['signDataObject', 'verifyDataObject', 'generateCapability', 'useCapability', 'encryptDataObject', 'decryptDataObject']
    rows = [[o] for o in operations]
    baseBenchmark = 'encryption_uncached'
    for (i, c) in enumerate(crossConfigs):
        benchmark = 'microbenchmark_%s_%s' % (baseBenchmark, c)
        result = results[benchmark]
        for (j, o) in enumerate(operations):
            rows[j].append('%.3f +- %.3f' % (result['specificTimes'][o]['mean'], result['specificTimes'][o]['dev']))

    rows.insert(0, [title] + crossConfigs[:])
    printSummaryTable(rows)

def printEndToEndTimeSummaryTable(title, results, baseBenchmarks, crossConfigs):
    rows = [[b] for b in baseBenchmarks]
    for (i, b) in enumerate(baseBenchmarks):
        for (j, c) in enumerate(crossConfigs):
            benchmark = 'microbenchmark_%s_%s' % (b, c)
            result = results[benchmark]
            rows[i].append('%.3f +- %.3f' % (result['endToEndTimes']['mean'], result['endToEndTimes']['dev']))
    
    rows.insert(0, [title] + crossConfigs[:])
    printSummaryTable(rows)

def printSizeSummaryTable(title, results, baseBenchmarks, crossConfigs):
    cats = ['doSizes', 'ndSizes', 'sdrSizes']
    rows = [[b] for b in baseBenchmarks]
    crossConfig = 'core_wo_cpulimit'
    for (i, b) in enumerate(baseBenchmarks):
        benchmark = 'microbenchmark_%s_%s' % (b, crossConfig)
        result = results[benchmark]
        for (j, c) in enumerate(cats):
            rows[i].append('%.0f +- %.0f => %.0f' % (result[c]['mean'], result[c]['dev'], result[c]['sum']))

    rows.insert(0, [title] + cats[:])
    printSummaryTable(rows)

def main():
    baseBenchmarks = ['base', 'signatures', 'encryption_cached', 'encryption_uncached']
    crossConfigs = ['core_wo_cpulimit', 'core_w_cpulimit', 'android']
    benchmarks = ['microbenchmark_%s_%s' % (b, c) for b in baseBenchmarks for c in crossConfigs]
    results = OrderedDict()

    for benchmark in benchmarks:
        results[benchmark] = getBenchmarkStats('microbenchmark', benchmark)

    print 'Individual Results:\n'
    for (k, v) in results.items():
        printTestResult(k, v)

    print '\n\nOperation Time Overhead Summary:\n\n'
    printOperationTimeSummaryTable('Time Overhead (s)', results, baseBenchmarks, crossConfigs)
    print '\n\nEnd To End Time Overhead Summary:\n\n'
    printEndToEndTimeSummaryTable('Time Overhead (s)', results, baseBenchmarks, crossConfigs)
    print '\n\nSpace Overhead Summary:\n\n'
    printSizeSummaryTable('Space Overhead (b)', results, baseBenchmarks, crossConfigs)

if __name__ == '__main__':
    arguments = docopt(__doc__)
    main()
