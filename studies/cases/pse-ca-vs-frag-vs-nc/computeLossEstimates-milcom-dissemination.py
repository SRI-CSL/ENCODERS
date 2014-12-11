#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import os
import glob
import shutil
from collections import OrderedDict

PCRE_PATH = '/home/lakhani/apps/pcre-8.35/pcregrep'

COMMAND = """cd %%DIR%%; %%PCRE_PATH%% -o1 -o2 "^[0-9]*\.[0-9]*:\[[0-9]*-[0-9]*\]{LossEstimateManager::lossEstimate}: Target node (n[0-9 ]*)has loss rate ([0-9]*\.[0-9]*), current tmpCount: [0-9]*" *.haggle.log | %%PCRE_PATH%% -o1 -o2 -o3 "^(n[0-9]*)\.haggle\.log(:n[0-9]* )([0-9]*\.[0-9]*)" > /tmp/lossEstimate-%%DIR%%.txt; cd ..""".replace('%%PCRE_PATH%%', PCRE_PATH)

NUM_NODES = 30
SQUADS = [
    ("squad1", [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
    ("squad2", [11, 12, 13, 14, 15, 16, 17, 18, 19, 20]),
    ("squad3", [21, 22, 23, 24, 25, 26, 27, 28, 29, 30])
]

def average(l):
    if len(l) == 0:
        return -1.0
    return sum(l) / len(l)

def getLossEstimateFileName(dir):
    return '/tmp/lossEstimate-%s.txt' % dir

def dumpLossEstimates(dir):
    if not os.path.exists(getLossEstimateFileName(dir)):
        print 'dumping loss estimate for %s' % dir
        # os.system('/bin/bash dumpLossEstimates.sh %s' % dir)
        os.system(COMMAND.replace('%%DIR%%', dir))
        # shutil.move('/tmp/lossEstimate.txt', getLossEstimateFileName(dir))
    # else:
        # print 'already have loss estimate for %s' % dir

def computePairwiseEstimates(dir):
    estimates = []
    with open(getLossEstimateFileName(dir)) as input:
        for line in input.readlines():
            nodes, val = line.split(' ')
            f, t = nodes.split(':')
            yield (f, t, float(val))

def createPairwiseEstimatesDict(numNodes, entries):
    items = [('n%s' % i, OrderedDict([('n%s' % j, []) for j in xrange(1, numNodes+1)])) for i in xrange(1, numNodes+1)]
    estimates = OrderedDict(items)
    for entry in entries:
        f, t, val = entry
        estimates[f][t].append(val)
    return estimates

def createPairwiseAverageEstimatesDict(estimates):
    return OrderedDict([(f, OrderedDict([(t, average(l)) for (t,l) in d.iteritems()])) for (f, d) in estimates.iteritems()])

def prettyPrintPairwiseEstimates(estimates):
    for (f, v) in estimates.iteritems():
        # print 'Estimates from %s:' % f
        for (t, e) in v.iteritems():
            print '    %s -> %s: %.4f' % (f, t, e)

def prettyPrintPairwiseEstimatesSingleLine(estimates):
    for (f, v) in estimates.iteritems():
        vs = ['(%s, %.4f)' % (t, e) for (t, e) in v.iteritems()]
        print '%s -> %s' % (f, vs)

def computePairwiseSquadEstimates(squads, estimates):
    items = [(name, OrderedDict([(name2, None) for (name2, n2s) in squads])) for (name, ns) in squads]
    output = OrderedDict(items)
    for (f, fs) in squads:
        for (t, ts) in squads:
            frs = ['n%s' % n for n in fs]
            tos = ['n%s' % n for n in ts]
            e = average([estimates[fr][to] for fr in frs for to in tos if estimates[fr][to] != -1])
            output[f][t] = e
    return output

def main():
    dirs = glob.glob('*_logs*')
    for dir in dirs:
        print 'Calculating loss estimate for %s' % dir
        dumpLossEstimates(dir)
        estimates = computePairwiseEstimates(dir)
        estimates = createPairwiseEstimatesDict(NUM_NODES, estimates)
        estimates = createPairwiseAverageEstimatesDict(estimates)
        # prettyPrintPairwiseEstimates(estimates)
        squadEstimates = computePairwiseSquadEstimates(SQUADS, estimates)
        prettyPrintPairwiseEstimatesSingleLine(squadEstimates)
        overallEstimates = computePairwiseSquadEstimates([("everyone", [i for i in xrange(1, NUM_NODES+1)])], estimates)
        prettyPrintPairwiseEstimatesSingleLine(overallEstimates)

if __name__ == '__main__':
    main()

