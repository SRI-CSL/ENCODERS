#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

# Factory for application modules. Loads them from
# the ApplicationModules/ directory. 

def get_application_module(cfg):
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
        module = __import__('ApplicationModules.' + name, fromlist=['ApplicationModules.' + name])
        return module.Application(parameters)
    except Exception, e:
        print e
        return None
