# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import sqlite3
import re
import traceback
import tombstones

class NodeAnalysis:
  def __init__(self,logpath,node,startTime):
    # Paths
    self.logpath = logpath
    self.nodepath = '%s/android-files/n%d' % (logpath, node)
    self.outputfile = '%s/outputfile' % self.nodepath
    self.logfile = '%s/haggle.log' % self.nodepath
    self.dbfile = '%s/haggle.db' % self.nodepath
    self.received_file = '%s/received_files.log' % self.nodepath
    self.startTime = startTime

    # Node#
    self.node=node

    # Data Objects received - Count "received" in outputfile
    self.parseOutputfile()

    # Total size of files received - From received_files.txt
    self.parseReceivedFiles()

    # Extract information from haggle.db
    self.parseDB()

    # Tombstone reports
    self.tombstones = self.parseTombstones()

    # Important config parameters - From config.xml
    self.importantConfigs = self.parseConfigs()

  def printStatisticsHeader(self):
    columnNames = ('Node', 'rcvDO', 'FileSize', 'CacheSize', 'Tombstones')
    print '%4s %8s %12s %12s %12s' % columnNames

  def printStatistics(self):
    columnValues = (self.node, self.receivedDataObjects, self.totalFileSize, self.cacheSize, len(self.tombstones))
    print '%4d %8d %12d %12d %12d' % columnValues

  def parseOutputfile(self):
    try:
      with open(self.outputfile) as f:
        contents = f.read()
        matches = re.findall(r'^Received,', contents, re.MULTILINE)
        self.receivedDataObjects = len(matches)
    except:
      self.receivedDataObjects = 0
      print 'Parsing outputfile failed for node %d' % self.node


  def parseReceivedFiles(self):
    def getFileSize(line):
      return int(line.split()[4])

    try:
      with open(self.received_file) as f:
        lines = f.readlines()
        self.receivedFiles = len(lines)
        self.totalFileSize = sum(map(getFileSize, lines))
    except:
      self.receivedFiles = self.totalFileSize = 0
      print 'Parsing received_files.log failed for node %d' % self.node

  def parseDB(self):
    conn = sqlite3.connect(self.dbfile)
    cursor = conn.cursor()
    cursor.execute('SELECT SUM(datalen), COUNT(datalen) FROM table_dataobjects')
    results = cursor.fetchall()

    if len(results) != 1 or results[0][0] == None:
      self.cacheSize = self.cacheCount = 0
    else:
      self.cacheSize = results[0][0]
      self.cacheCount = results[0][1]

    conn.close()

  def parseTombstones(self):
    sysLibRoot = self.logpath + "/system/lib"
    mapsPath = self.nodepath + "/maps"
    tombstonePath = self.nodepath + "tombstones"

    try:
      return tombstones.Tombstones(sysLibRoot, mapsPath, self.logfile, tombstonePath, self.startTime)
    except:
      print 'An exception occurred when trying to parse tombstones.'
      print 'Is "arm-linux-androideabi-addr2line" in your PATH?'
      return []

  def parseConfigs(self):
    return []

