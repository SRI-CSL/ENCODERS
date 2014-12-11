#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

### Keep this in sync with the ccb.py in phase2-basic-security branch's ccb/python directory

import os, io, sys
import pickle

try:
    import emb
    class Catcher(io.TextIOBase):
        def __init__(self):
            pass
        def write(self, stuff):
            emb.log(stuff)
    sys.stdout = Catcher()
    sys.stderr = Catcher()
except ImportError:
    def emblogger(s):
        print s
    from collections import namedtuple
    mock = namedtuple('mock', ['log'])
    emb = mock(log=emblogger)

from charm.schemes import dabe_aw11
from charm.adapters import dabenc_adapt_hybrid
from charm.toolbox.pairinggroup import PairingGroup, G1
from charm.core.engine import util

state = {}

def serialize(item):
    return util.objectToBytes(item, state['groupObj'])

def deserialize(item):
    return util.bytesToObject(item, state['groupObj'])

def serializeItems(lis):
    return [(k,serialize(v)) for (k,v) in lis]

def init(persistence=b''):
    global state
    state = {}

    try:
        if persistence is None or persistence == b'':
            emb.log('ERROR: No persistence value provided. Things may not work correctly.\n')
        else:
            emb.log('Using passed in persistence value.\n')

            state['groupObj'] = PairingGroup('SS512')
            tmp = util.bytesToObject(persistence, state['groupObj'])
            state.update(tmp)

            state['dabe'] = dabe_aw11.Dabe(state['groupObj'])
            state['hyb_abema'] = dabenc_adapt_hybrid.HybridABEncMA(state['dabe'], state['groupObj'])
            state['gp']['H'] = lambda x: state['groupObj'].hash(x, G1)

            emb.log('Loaded state.\n')
            return

    except Exception:
        emb.log('ERROR: Exception while trying to load persistence value. Things may not work correctly.\n')

    state['groupObj'] = PairingGroup('SS512')
    state['dabe'] = dabe_aw11.Dabe(state['groupObj'])
    state['hyb_abema'] = dabenc_adapt_hybrid.HybridABEncMA(state['dabe'], state['groupObj'])
    state['gp'] = state['hyb_abema'].setup()
    state['authority_sk'] = {}
    state['authority_pk'] = {}
    state['user_sk'] = {}

    emb.log('Created state\n')

def shutdown():
    group = state['groupObj']
    del state['gp']['H']
    del state['dabe']
    del state['groupObj']
    del state['hyb_abema']
    ret = util.objectToBytes(state, group)

    return ret

# It is up to the caller to ensure that attributes are unique
def authsetup(attributes):
    attributes = map(str.upper, attributes)
    sk, pk = state['hyb_abema'].authsetup(state['gp'], attributes)
    state['authority_sk'].update(sk)
    state['authority_pk'].update(pk)

    sk, pk = (serializeItems(sk.items()), serializeItems(pk.items()))
    return (sk, pk)

def keygen(i, gid, save):

    dic = {}
    if save == b'True':
        dic = state['user_sk']

    key = state['hyb_abema'].keygen(state['gp'], state['authority_sk'], i.upper(), gid, dic)
    return serialize(key)

def encrypt(M, policy):

    #ct = state['hyb_abema'].encrypt(state['authority_pk'], state['gp'], M.decode('ascii'), policy.decode('ascii'))
    ct = state['hyb_abema'].encrypt(state['authority_pk'], state['gp'], M, policy.upper())
    return serialize(ct)

def decrypt(ct):

    ct = deserialize(ct)
    #pt = state['hyb_abema'].decrypt(state['gp'], state['user_sk'], ct).encode('ascii')
    pt = state['hyb_abema'].decrypt(state['gp'], state['user_sk'], ct)
    return pt

def set_value(dict, key, value, serialized):
    if serialized == b'True':
        key = key.upper()
        value = deserialize(value)

    state[dict][key] = value

