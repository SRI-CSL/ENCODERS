# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# A custom mobility module. This is very custom tailored and specific
# to a certain scenario (it is probably not useful outside of this 
# scenario).
#

import io
import re
import random
import math

class Node(object):

    def __init__(self, name, x, max_x, y, max_y, radius, randomInstance):
        self.name = name
        self.x = x
        self.max_x = max_x
        self.y = y
        self.max_y = max_y
        self.radius = radius
        self.randomInstance = randomInstance

    def setposition(self, x, y):
        self.x = x
        self.y = y

    def getposition(self):
        return (self.x, self.y)

    def moverandom(self):
        theta = self.randomInstance.random() * 2 * math.pi
        self.x = math.fabs(int(self.x + self.radius * math.cos(theta)))
        if self.x > self.max_x:
            self.x = 2 * self.max_x - self.x
            self.y = math.fabs(int(self.y + self.radius * math.sin(theta)))
        if self.y > self.max_y:
            self.y = 2 * self.max_y - self.y

def generate_scen_file_helper(scen_output, num_nodes, duration, area_width, area_height, seed, speed, radius, dwelltime, delay=0):
    of = open(scen_output + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (num_nodes, duration, area_width, area_height))
    randomInstance = random.Random()
    randomInstance.seed(seed)
    nodelist = []
    for i in range(1, num_nodes+1):
        x = randomInstance.random()*area_width
        y = randomInstance.random()*area_height
        of.write("$node_(%d) set X_ %.2f\n" % (i, x))
        of.write("$node_(%d) set Y_ %.2f\n" % (i, y))
        nodelist.append(Node("n%d" % i, x, area_width, y, area_height, radius, randomInstance))
    cur = delay
    traveltime = radius / speed
    timeleft = duration
    while cur <= duration:
        for node in nodelist:
            node.moverandom()
            of.write("$ns_ at %d.00 \"$node_(%s) setdest %.2f %.2f %.2f\"\n" % (cur, node.name[1:], node.x, node.y, speed))
        cur = cur + dwelltime
    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = self.cfg['num_nodes']
            duration = self.cfg['duration']
            area_width = self.cfg['area_width']
            area_height = self.cfg['area_height']
            seed = self.cfg['seed']
            speed = self.cfg['speed']
            radius = self.cfg['radius']
            dwelltime = self.cfg['dwelltime']
            try:
                delay = float(self.cfg['mobility_delay_s'])
            except:
                delay = 0
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, seed, speed, radius, dwelltime, delay)
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
        pretty += "sv"
        pretty += "-"
        pretty += "r%d" % self.cfg['radius']
        pretty += "v%d" % self.cfg['speed']
        pretty += "p%d" % self.cfg['dwelltime'] 
        pretty += "s%d" % self.cfg['seed']
        return pretty
