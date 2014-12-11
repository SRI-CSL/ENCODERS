#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

# Mobility module factory. It loads the modules specified in the
# MobilityModules/ directory.

def get_mobility_module(cfg):
    try:
    	name = cfg['name']
    	parameters = cfg['parameters']
        try:
            existing_pretty = parameters['pretty_name']
        except:
            try:
                parameters['pretty_name'] = cfg['pretty_name']
            except:
                None
        module = __import__('MobilityModules.' + name, fromlist=['MobilityModules.' + name])
        if not module:
            print "No mobility module found"
            raise Exception()
        return module.Mobility(parameters)
    except Exception, e:
        print e
        return None
