# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# A custom mobility module. This is very custom tailored and specific
# to a certain scenario (it is probably not useful outside of this 
# scenario).
#
# Srinivas' static scenario 1 (6 node)

import os 
import math
import sys


class Position(object):
    def __init__(self, x, y):
        self.x = x
        self.y = y

def generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, pub_velocity, sub_velocity, delay=0):

    of = open(output_path + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (num_nodes, duration, area_width, area_height))

    of.write("$node_(1) set X_ 151.0\n")
    of.write("$node_(1) set Y_ 181.0\n")
    of.write("$node_(2) set X_ 437.0\n")
    of.write("$node_(2) set Y_ 71.0\n")
    of.write("$node_(3) set X_ 204.0\n")
    of.write("$node_(3) set Y_ 477.0\n")
    of.write("$node_(4) set X_ 540.0\n")
    of.write("$node_(4) set Y_ 520.0\n")
    of.write("$node_(5) set X_ 758.0\n")
    of.write("$node_(5) set Y_ 172.0\n")
    of.write("$node_(6) set X_ 798.0\n")
    of.write("$node_(6) set Y_ 460.0\n")

    # Publisher mobility
    positionlist1 = []
    positionlist1.append(Position(437, 130))
    positionlist1.append(Position(200, 250))
    positionlist1.append(Position(250, 410))
    positionlist1.append(Position(600, 450))
    positionlist1.append(Position(758, 172))
    cur = delay

    sx = 758
    sy = 172
    for i in positionlist1:
        of.write("$ns_ at %d.00 \"$node_(5) setdest %d.0 %d.0 %d.0\"\n" % (cur, i.x, i.y, pub_velocity))
        cur += math.sqrt((i.x - sx)*(i.x - sx) + (i.y - sy) * (i.y - sy)) / pub_velocity
        sx = i.x
        sy = i.y

    # Subscriber mobility
    positionlist2 = []
    positionlist2.append(Position(437, 130))
    positionlist2.append(Position(200, 250))
    positionlist2.append(Position(250, 410))
    positionlist2.append(Position(600, 450))
    positionlist2.append(Position(798, 460))

    cur = cur + 5

    sx = 798
    sy = 460
    for i in positionlist2:
        of.write("$ns_ at %d.00 \"$node_(6) setdest %d.0 %d.0 %d.0\"\n" % (cur, i.x, i.y, sub_velocity))
        cur += math.sqrt((i.x - sx)*(i.x - sx) + (i.y - sy) * (i.y - sy)) / sub_velocity
        sx = i.x
        sy = i.y
    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = int(self.cfg['num_nodes'])
            if num_nodes != 6:
                raise Exception("incorrect number of nodes")
            duration = int(self.cfg['duration'])
            area_width = int(self.cfg['area_width'])
            if area_width != 800:
                raise Exception("incorrect width: %d" % area_width)
            area_height = int(self.cfg['area_height'])
            if area_height != 600:
                raise Exception("incorrect height; %d" % area_height)
            pub_velocity = float(self.cfg['pub_velocity'])
            sub_velocity = float(self.cfg['sub_velocity'])
            try:
                delay = float(self.cfg['mobility_delay_s'])
            except:
                delay = 0
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration"
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, pub_velocity, sub_velocity, delay)
            return True
        except Exception, e:
            print e
            print "Invalid mobility configuration"
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "sv-static1"
        if verbosity >= 2:
            pretty += "-v%.2f,%.2f" % (float(pub_velocity), float(sub_velocity))
        return pretty
