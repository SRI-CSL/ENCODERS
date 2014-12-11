#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import sys
import argparse
import json
import re
import glob
from datetime import datetime

from nodeanalysis import NodeAnalysis
from haggleloganalyzer import HaggleLogAnalyzer
# from tombstones import Tombstones

class TestReport:
  def __init__(self, options):
    self.options = options
    self.logpath = options.logpath
    self.greplist = options.greplist

    if self.greplist:
      self.greplog = self.greplist.replace('greplist', 'greplog')

    # Comment (from JSON)
    self.comment = self.getComment()

    # Commit# (from haggle log)
    self.commit = self.getCommit()

    # Test times
    self.startTime, self.endTime, self.duration = self.getTimes()

    # Node analysis
    self.nodes = self.getNodeAnalysis()
    self.nodeCount = len(self.nodes)

    # Aggregate statistics
    self.maxCacheSize = self.totalDOReceived = self.totalFilesReceived = 0
    self.totalFileSize = self.totalCacheSize = 0
    self.tombstones = []

    for node in self.nodes:
      self.tombstones += node.tombstones
      self.maxCacheSize = max(self.maxCacheSize, node.cacheSize)
      self.totalDOReceived += node.receivedDataObjects
      self.totalFileSize += node.totalFileSize
      self.totalFilesReceived += node.receivedFiles
      self.totalCacheSize += node.cacheSize

    self.grepResults = self.grepLogs()

  def getComment(self):
    try:
      with open(self.logpath + "/test.json") as f:
        jsonObject = json.loads(f.read())
        return jsonObject['pretty_settings']['comment']
    except:
      return None

  def getCommit(self):
    try:
      with open(self.logpath + "/n1.haggle.log") as f:
        result = re.search(r'Git reference is (\w+)', f.read())
        return result.group(1)
    except:
      return 'Unknown'

  def getTimes(self):
    try:
      with open(self.logpath + "/time.log") as f:
        timeLines = f.readlines()
        startTime = datetime.fromtimestamp(int(timeLines[0]))
        endTime = datetime.fromtimestamp(int(timeLines[1]))
        duration = endTime - startTime
        return [startTime, endTime, duration]
    except:
      return ['Unknown', 'Unknown', 'Unknown']

  def getNodeAnalysis(self):
    def analyzeNode(n):
      return NodeAnalysis(self.logpath, n, self.startTime)

    count = len(glob.glob(self.logpath + "/*.haggle.log"))
    nodes = map(analyzeNode, range(1, count+1))

    return nodes

  def grepLogs(self):
    if self.greplist:
      return HaggleLogAnalyzer(self.logpath, self.greplist, self.greplog)
    else:
      return None

  def printReport(self):
    self.printSummary()
    print ""
    self.printNodeStatistics()
    print ""
    self.printTombstones()
    print ""
    if(self.grepResults):
      self.printGrepResults()

  def printSummary(self):
    print "Summary"
    print "======="
    print "  Comment: %s" % self.comment
    print "  Commit: %s" % self.commit
    print "  Test Time: %s ~ %s (duration: %ss)" % (self.startTime, self.endTime, self.duration)
    print "  Node Count: %d" % self.nodeCount
    print "  Max Cache Size: %d" % self.maxCacheSize
    print "  Number of Tombstones: %d" % len(self.tombstones)
    print "  Data Objects Received: %d" % self.totalDOReceived

  def printNodeStatistics(self):
    if len(self.nodes) == 0:
      return
    print "Node Statistics"
    print "==============="
    self.nodes[0].printStatisticsHeader()
    for node in self.nodes:
      node.printStatistics()
    print ""
    print "  Number of Data Objects: %d" % self.totalDOReceived
    if self.nodeCount > 0:
      print "  Average total file size: %d bytes" % (self.totalFileSize / self.nodeCount)
    if self.nodeCount > 0:
      print "  Average DB footprint: %d bytes" % (self.totalCacheSize / self.nodeCount)

  def printTombstones(self):
    print "Tombstones"
    print "=========="
    for tombstone in self.tombstones:
      print tombstone.toString()

  def printGrepResults(self):
    print "Grep Results"
    print "============"
    print self.grepResults.toString()

def parseOptions(argv):
  parser = argparse.ArgumentParser(description='Generate a report from an AndroidTestRunner test run.')
  parser.add_argument('logpath', metavar='PATH', help='The directory containing the logs for the test run.')
  parser.add_argument('-g', metavar='GREPLIST', dest='greplist', help='A file containing a newline-separated list of terms to grep for in the haggle logs.')
  return parser.parse_args()

def main():
  report = TestReport(parseOptions(sys.argv))
  report.printReport()

if __name__ == '__main__':
  main()
