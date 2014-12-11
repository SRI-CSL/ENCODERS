#
# Copyright (c) 2014 SRI International and GPC
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Yu-Ting Yu (YTY)
#   Hasnain Lakhani (HL)


#
# A mobility module created for vignette1. The 
# node iterate over a list of reference points. 
#

import io
import re
import math
import random
import os

def pythag(position_a_x, position_a_y, position_b_x, position_b_y):
    width = abs(position_a_x - position_b_x)
    height =abs(position_a_y - position_b_y)
    return math.sqrt(width ** 2 + height ** 2)

def read_nodes_file(input_path):
    
    input_path = os.path.abspath(input_path)
    print "generate movement by ", input_path, "\n"
    return_list = []
    circle_list = []

    f = open(input_path, 'r')
    for line in f:
        if len(line.strip()) > 0 and line[0]!='#':
            line = line.strip('\n').split(", ", 1)
            time = float(line[0])
            if time == 0:
                #start point
                return_list.append(eval(line[1]))
            else:                
                #to iterate through
                circle_list.append((time, eval(line[1])))

    return_list.append(circle_list)                        
    return return_list

def generate_node_offset(num_nodes, random_seed, max_dist_to_ref):
    offset = []
    random.seed(random_seed)      
    for x in range(0, num_nodes):
        generated = False

        while(generated != True):
            x_offset = random.random() * max_dist_to_ref
            if(random.random() < 0.5):
                x_offset = x_offset * (-1)
            y_offset = random.random() * max_dist_to_ref
            if(random.random() < 0.5):
                y_offset = y_offset * (-1)
            distance = pythag(x_offset, y_offset, 0,0)
            if(distance <= max_dist_to_ref):
                offset.append((x_offset, y_offset))
                generated = True

    return offset

def generate_scen_file_helper(output_path, duration, area_width, area_height, reference_point_file, num_nodes, random_seed, max_dist_to_ref):
    of = open(output_path + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (1, duration, area_width, area_height))
    
    pointlist = read_nodes_file(reference_point_file)
    nodeoffsets = generate_node_offset(num_nodes, random_seed, max_dist_to_ref)

    #iterate through reference points
    
    fudge = 0
    area_width = area_width - fudge
    area_height = area_height - fudge    

    [start_x, start_y] = pointlist[0]

    for node in range(0, len(nodeoffsets)):
        [offset_x, offset_y] = nodeoffsets[node]
        of.write("$node_(%d) set X_ %.2f\n" % (node + 1, start_x + offset_x))
        of.write("$node_(%d) set Y_ %.2f\n" % (node + 1, start_y + offset_y))

    for node in range(0, len(nodeoffsets)):
        [offset_x, offset_y] = nodeoffsets[node]
        of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (0, node + 1, start_x + offset_x, start_y + offset_y, 0))
            
    cur_time = 0.0
    prev_time = 0.0
    prev_pos_x = start_x
    prev_pos_y = start_y
    cycle_start_time = 0.0
    pointcycle = pointlist[1]
    
    i = 0
    while cur_time <= duration:
        [time, point] = pointcycle[i]
        [ref_x, ref_y] = point

        next_checkpoint_arrival_time = cycle_start_time + time
       
        if (next_checkpoint_arrival_time - cur_time) <= 1:
            #set to next checkpoint
            cur_time = next_checkpoint_arrival_time
            distance = pythag(ref_x, ref_y, prev_pos_x, prev_pos_y)
            velocity = distance / (cur_time - prev_time)

            for node in range(0, len(nodeoffsets)):
                [offset_x, offset_y] = nodeoffsets[node]
                of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (cur_time, node + 1, ref_x + offset_x, ref_y + offset_y, velocity))
 
            prev_time = cur_time
            prev_pos_x = ref_x
            prev_pos_y = ref_y
            
            #advance checkpoint
            i = i + 1
            if i >= len(pointcycle):
                i = 0
                cycle_start_time = cur_time
        else:
            x = prev_pos_x + (ref_x - prev_pos_x) / (next_checkpoint_arrival_time - prev_time)
            y = prev_pos_y + (ref_y - prev_pos_y) / (next_checkpoint_arrival_time - prev_time)
            cur_time += 1
            distance = pythag(x, y, prev_pos_x, prev_pos_y)
            velocity = distance / (cur_time - prev_time)

            for node in range(0, len(nodeoffsets)):
                [offset_x, offset_y] = nodeoffsets[node]
                of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (cur_time, node + 1, x + offset_x, y + offset_y, velocity))

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
            reference_point_file = self.cfg['reference_point_file']
            random_seed = int(self.cfg['seed'])
            max_dist_to_ref = float(self.cfg['max_dist_to_ref'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, duration, area_width, area_height, reference_point_file, num_nodes, random_seed, max_dist_to_ref)
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
        pretty += "vignette1"
        pretty += "%dx%d" % (int(self.cfg['area_width']), int(self.cfg['area_height']))
        return pretty
