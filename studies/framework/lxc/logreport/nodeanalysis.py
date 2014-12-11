# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import sqlite3
import re
import traceback
import glob

class NodeAnalysis:
  def __init__(self,logpath,node,startTime):
    # Paths
    self.logpath = logpath
    self.nodepath = '%s/n%d' % (logpath, node)
    self.outputfile = ''
    self.logfile = '%s.haggle.log' % self.nodepath
    self.dbfile = '%s/db/n%d.haggle.db' % (logpath, node)
    self.received_file = '%s/received_files.log' % self.nodepath
    self.startTime = startTime

    for ofile in glob.glob('%s/../*.n%d' % (logpath, node)):
        self.outputfile = ofile

    # Node#
    self.node=node

    # Data Objects received - Count "received" in outputfile
    self.parseOutputfile()

    # Total size of files received - From received_files.txt
    self.parseReceivedFiles()

    # Extract information from haggle.db
    self.parseDB()

    # Important config parameters - From config.xml
    self.importantConfigs = self.parseConfigs()

  def printStatisticsHeader(self):
    columnNames = ('Node', 'rcvDO', 'FileSize', 'CacheSize', 'Tombstones')
    print '%4s %8s %12s %12s %12s' % columnNames

  def printStatistics(self):
    columnValues = (self.node, self.receivedDataObjects, self.totalFileSize, self.cacheSize)
    print '%4d %8d %12d %12d' % columnValues

  def parseOutputfile(self):
    try:
      with open(self.outputfile) as f:
        contents = f.read()
        matches = re.findall(r'^Received,', contents, re.MULTILINE)
        self.receivedDataObjects = len(matches)
    except:
      self.receivedDataObjects = 0
      print 'Parsing outputfile %s failed for node %d' % (self.outputfile, self.node)


  def parseReceivedFiles(self):
    def getFileSize(line):
      return int(line.split(',')[7])

    try:
      with open(self.outputfile) as f:
        contents = f.read()
        lines = re.findall(r'^Received,[0-9a-zA-z,\.\(\)]*', contents, re.MULTILINE)
        self.receivedFiles = len(lines)
        self.totalFileSize = sum(map(getFileSize, lines))
    except:
      self.receivedFiles = self.totalFileSize = 0 
      import traceback
      traceback.print_exc()
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

  def parseConfigs(self):
    return []

