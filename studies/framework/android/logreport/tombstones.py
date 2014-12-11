# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import glob
import re
import os.path
from subprocess import check_output
from datetime import datetime

def main():
  # 130308-2230-config-flood-direct-coding-udptcp-bcast-rlimit_IA2-V-2_received-5730-OTA
  logroot = "test"
  libsRoot = logroot + "/android-system-lib"
  mapsPath = logroot + "/maps"
  tombstoneRoot = logroot + "/tombstones"
  hagglelog = logroot + "/usb9.log"

  t = Tombstones(libsRoot, mapsPath, hagglelog, tombstoneRoot, datetime.fromtimestamp(0))
  print t.toString()

class Tombstones:
  ARM_ADDR2LINE = "arm-linux-androideabi-addr2line"
  LOG_LINE_NUM_TO_SAVE = 4
  printPCAddress = False
  printEntireTombstone = False

  tomblist = []

  def __init__(self, sysLibRoot, mapsPath, hagglelog, tombstonePath, start):
    self.addr2linepath = check_output("which %s" % self.ARM_ADDR2LINE, shell=True) != ''

    self.libsRoot = sysLibRoot
    self.mapsPath = mapsPath
    self.tombstonePath = tombstonePath
    self.hagglelog = hagglelog
    self.start = start
    self.buildTombstoneList()
    self.analyzeTombstones()

    #self.printToFile("typescript")

  def __getitem__(self, index):
    if index < len(self.tomblist):
      return self.tomblist[index]
    else:
      return None

  def buildTombstoneList(self):
    """Build a list of tombstones, including their creation time."""

    def buildTombTemplate(file):
      return {'file': file, 'ctime': datetime.fromtimestamp(os.path.getctime(file))}

    def rejectOldTombstones(tomb):
      return tomb['ctime'] > self.start

    files = glob.glob(self.tombstonePath + "/tombstone_*")
    self.tomblist = filter(rejectOldTombstones, map(buildTombTemplate,files))


  def analyzeTombstones (self) :
    if len(self.tomblist) == 0:
      return
    tidp = re.compile(r"tid:\s(\d+)")
    pcp = re.compile(r"pc\s([\d\w]+)")

    for entry in self.tomblist:
      libinfo = None
      tid = None
      log = None

      with open(entry['file'], "rb") as f:
        stone = f.read()

        ret = tidp.findall(stone)
        if len(ret) == 1:
          tid = ret[0]
          log = self.checklog(tid)
        ret = pcp.findall(stone)
        if len(ret) == 1:
          pc = ret[0]
          libinfo = self.checklib(pc)

      entry['tid'] = tid
      entry['libinfo'] = libinfo
      entry['log'] = log

  def toString(self):
    text = "Number of Tombstones = "+str(len(self.tomblist))+"\n\n"
    for entry in self.tomblist:
      tombstonef = open(entry['file'], "rb")
      elapsed = entry['ctime'] - self.start

      # print tombstone title
      text += entry['file']
      text += "\n--------------------------------\n  "
      text += "creation time = %s (Tombstone is generated after %s)\n  " % (entry['ctime'], elapsed)

      # print lib info
      if entry['libinfo'] == None:
        text += "[ERROR]Tombstone is empty.\n\n"
      else:
        text += entry['libinfo'].replace("\n", "\n  ")+"\n   "

      # print tid info
      if entry['log'] != None:
        text += "\n   ".join(entry['log'][-self.LOG_LINE_NUM_TO_SAVE:])

      # print tombstone
      if self.printEntireTombstone:
        text += "\n  "
        for line in tombstonef:
          text += line.replace("\n", "\n  ")
        text += "\n"
    return text

  def checklog (self, tid):
    """Return a list of the lines produced by a specified
    thread ID in a the node's haggle log."""

    tidp = re.compile(r":\[%s-" % tid)

    with open(self.hagglelog, 'rb') as f:
      return [line.strip() for line in f.readlines() if tidp.search(line)]

  def fromHex(self, string):
    return int(string,16)

  def checklib (self, pc):
    p = re.compile(r"([\d\w]+)-([\d\w]+) .{4} .{8} ..:.. \d+ .*lib/([\d\w\+]+\.so)")
    # p = re.compile(r"([\d\w]+)-([\d\w]+)")
    # p2 = re.compile(r"lib/([\d\w\+]+\.so)")
    with open(self.mapsPath, "rb") as maps:
      for line in maps:
        ret = p.match(line)
        if ret:
          s   = ret.group(1)
          e   = ret.group(2)
          lib = ret.group(3)
          offset = int(pc,16)-int(s,16)

          if s <= pc and pc <= e:
            if self.printPCAddress:
              text =  "PC(%s) is in the memory(%s)\n" % (pc, line.strip())
              text = text + self.addr2line(offset, lib)
            else:
              text = self.addr2line(offset, lib)
            return text
    return "[NO address(" + pc + ") in the memory map]"

  def addr2line (self, addr, lib) :
    if not self.addr2linepath:
      return '  [ERROR] : There is no path for %s' % ARM_ADDR2LINE

    lib = '%s/%s' % (self.libsRoot, lib)
    cmd = '%s -f -C -e %s %s' % (self.ARM_ADDR2LINE, lib, str(hex(addr))[2:])
    return check_output(cmd, shell=True).replace("\n", "\n   ")

if __name__ == '__main__':
  main()
