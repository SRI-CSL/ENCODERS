#
# Copyright (c) 2014 SRI International and GPC
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Yu-Ting Yu (YTY)
#   Hasnain Lakhani (HL)


#
# A mobility module created for data mule patrol scenario. The 
# node moves along the line between reference points. 
#

import io
import re
import math

def pythag(position_a_x, position_a_y, position_b_x, position_b_y):
    width = abs(position_a_x - position_b_x)
    height =abs(position_a_y - position_b_y)
    return math.sqrt(width ** 2 + height ** 2)

def generate_scen_file_helper(output_path, duration, area_width, area_height, reference_1_x, reference_1_y, reference_2_x, reference_2_y, velocity, stay):
    of = open(output_path + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (1, duration, area_width, area_height))

    fudge = 0
    area_width = area_width - fudge
    area_height = area_height - fudge

    of.write("$node_(%d) set X_ %.2f\n" % (1, (reference_1_x + reference_2_x) /2))
    of.write("$node_(%d) set Y_ %.2f\n" % (1, (reference_1_y + reference_2_y) /2))

    cur_time = 0
    prev_time = 0
    prev_pos_x = (reference_1_x+reference_2_x) / 2
    prev_pos_y = (reference_1_y+reference_2_y) / 2
    direction_reverse = False #True: from ref1 to ref2, False: from ref2 to ref1

    while cur_time <= duration:
        if(direction_reverse):
            ref_x = reference_1_x
            ref_y = reference_1_y
        else:
            ref_x = reference_2_x
            ref_y = reference_2_y

        distance = pythag(prev_pos_x, prev_pos_y, ref_x, ref_y)
        time = distance / velocity
        if time < 1:
            cur_time += time
            of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (cur_time, 1, ref_x, ref_y, velocity))
            cur_time += stay
            of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (cur_time, 1, ref_x, ref_y, velocity))
            prev_pos_x = ref_x
            prev_pos_y = ref_y
            prev_time = cur_time
            #reverse as we have reached the end point
            direction_reverse = ~direction_reverse
        else:
            #print the position of the next second
            x = prev_pos_x + (ref_x - prev_pos_x)/time
            y = prev_pos_y + (ref_y - prev_pos_y)/time
            cur_time+=1
            of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (cur_time, 1, x, y, velocity))
            prev_pos_x = x
            prev_pos_y = y
            prev_time = cur_time

    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            duration  = int(self.cfg['duration'])
            num_nodes = int(self.cfg['num_nodes'])
            if num_nodes != 1:
                raise Exception("invalid number of nodes")
            reference_1_x = float(self.cfg['reference_1_x'])
            reference_1_y = float(self.cfg['reference_1_y'])
            reference_2_x = float(self.cfg['reference_2_x'])
            reference_2_y = float(self.cfg['reference_2_y'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            velocity = float(self.cfg['velocity'])
            stay = float(self.cfg['stay'])
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, duration, area_width, area_height, reference_1_x, reference_1_y, reference_2_x, reference_2_y, velocity, stay)
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
        pretty += "datamule"
        pretty += "-"
        pretty += "%.2f-" % (int(self.cfg['reference_1_x']),)
        pretty += "%.2f-" % (int(self.cfg['reference_1_y']),)
        pretty += "%.2f-" % (int(self.cfg['reference_2_x']),)
        pretty += "%.2f-" % (int(self.cfg['reference_2_y']),)
        pretty += "%.2f-" % (float(self.cfg['velocity']),)
        pretty += "%dx%d" % (int(self.cfg['area_width']), int(self.cfg['area_height']))
        return pretty
