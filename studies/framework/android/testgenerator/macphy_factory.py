#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Joshua Joy (JJ)
#   Hasnain Lakhani (HL)

class MacPhy:
    def __init__(self, cfg, nodes, num_nodes, macphy_num):
        self.macphy_num = macphy_num
        self.num_nodes = int(num_nodes)
        self.cfg = cfg
        self.nodes = nodes

    def has_interface(self, node_num):
        return node_num in self.nodes

    def get_custom_cfg(self):
        return ""

    def get_custom_cfg_name(self):
        return ""

    def get_nodes(self):
        return self.nodes

    def get_macphy_num(self):
        return self.macphy_num

    def get_node_num(self):
        return self.macphy_num + self.num_nodes

    def get_node_cfg(self):
        node_cfg_str = ""
        node_cfg_str += "node n%d {\n" % (self.get_node_num(),)
        node_cfg_str += "    type wlan\n"
        node_cfg_str += "    network-config {\n"
        node_cfg_str += "\thostname wlan%d\n" % self.macphy_num
        node_cfg_str += "\t!\n"
        node_cfg_str += "\tinterface wireless\n"
        node_cfg_str += "\t ip address 10.0.%d.0/32\n" % (self.get_macphy_num(), )
        #node_cfg_str += "\t ipv6 2001:%d::0/128\n" % (self.get_macphy_num(), )
        node_cfg_str += "\t ipv6 address 2001:%d::0/128\n" % (self.get_macphy_num(), )
        node_cfg_str += "\t!\n"
        #if self.macphy_num == 1:
            #node_cfg_str += "\tscriptfile\n"
            #node_cfg_str += "\t%s\n" % "%%scen_path%%"
            #node_cfg_str += "\t!\n"
        node_cfg_str += "\tmobmodel\n"
        node_cfg_str += "\tcoreapi\n"
        #node_cfg_str += "\tns2script\n"
        node_cfg_str += "\t%s\n" % self.get_custom_cfg_name()
        node_cfg_str += "\tns2script\n"
        node_cfg_str += "\t!\n"
        node_cfg_str += "    }\n"
        node_cfg_str += self.get_custom_cfg()
        node_cfg_str += self.get_ns2script()
        node_cfg_str += "    canvas c1\n"
        node_cfg_str += "    iconcoords {0.0 0.0}\n"
        node_cfg_str += "    labelcoords {0.0 0.0}\n"
        for i in range (0, len(self.nodes)):
            node_cfg_str += "    interface-peer {e%d n%d}\n" % (i, self.nodes[i])
        node_cfg_str += "}\n"
        return node_cfg_str

    def get_iface_str(self, node_num, iface_count):
        iface_str = ""
        iface_str += "\tinterface eth%d\n" % iface_count
        iface_str += "\t ip address 10.0.%d.%d/32\n" % (self.get_macphy_num(), node_num + 19)
        iface_str += "\t ipv6 address 2001:%d::%d/128\n" % (self.get_macphy_num(), node_num + 19)
        return iface_str

    def get_interface_peer(self, node_num, iface_count):
        iface_str = '    interface-peer {%d n%d}\n' % (iface_count, self.get_node_num())
        return iface_str

    def get_ns2script(self):
        ctemplate="""    custom-config {
	custom-config-id ns2script
	custom-command {10 3 11 10 10 10 10 10}
	config {
	file=%%scen_path%%
	refresh_ms=50
	loop=0
	autostart=0.0
	map=
	script_start=
	script_pause=
	script_stop=
	}
    }
"""
	#vals = {'scenfile':scenfile}
	#config-ns2 = ctemplateformat(**vals)
	return ctemplate

class MacPhyBasic(MacPhy):

    def get_custom_cfg_name(self):
        return "basic_range"

    def get_custom_cfg(self):
        ctemplate="""    custom-config {{
	custom-config-id basic_range
	custom-command {{3 3 9 9 9}}
	config {{
	range={range_radius}
	bandwidth={bandwidth_bps}
	jitter=0
	delay={latency_us}
	error={pkt_error_rate}
	}}
    }}
"""

        vals =  {
                    'range_radius':int(self.cfg['range_radius_m']),
                    'bandwidth_bps':int(self.cfg['bandwidth_bps']),
                    'latency_us':int(self.cfg['latency_us']),
                    'pkt_error_rate':float(self.cfg['pkt_error_rate'])
                }
        ctemplateformatted = ctemplate.format(**vals)
        return ctemplateformatted

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "basic_mac"
        pretty += "-"
        if verbosity >= 1:
            pretty += "r%d" % int(self.cfg['range_radius_m'])
            pretty += "c%d" % (int(self.cfg['bandwidth_bps'])/(1000*1000),)
            pretty += "d%d" % int(self.cfg['latency_us'])
            pretty += "err%.2f" % float(self.cfg['pkt_error_rate'])
        return pretty

class MacPhyEmaneRFPipe(MacPhy):

    def get_custom_cfg_name(self):
        return "emane_rfpipe"

    def get_custom_cfg(self):
        ctemplate="""    custom-config {{
	custom-config-id emane_rfpipe
	custom-command {{11 3 9 9 11 2 11 10 10 9 9 9 10 10 11 4 11 4 4 11 11 10 2 9 9}}
	config {{
	enablepromiscuousmode=0
	datarate={datarate}
	jitter={jitter}
	delay={delay}
	flowcontrolenable=0
	flowcontroltokens=10
	enabletighttiming=0
	pcrcurveuri=/usr/share/emane/models/rfpipe/xml/rfpipepcr.xml
	transmissioncontrolmap=
	antennagain={antennagain}
	antennaazimuth=0.0
	antennaelevation=0.0
	antennaprofileid=1
	antennaprofilemanifesturi=
	antennaprofileenable=0
        antennatype={antennatype}
	bandwidth={bandwidth_khz}
	defaultconnectivitymode=0
	frequency=2.347G
	frequencyofinterest=2.347G
	frequencyofinterestfilterenable=1
	noiseprocessingmode=0
	pathlossmode={pathlossmode}
	subid=1
	systemnoisefigure={system_noise_figure}
	txpower={txpower}
	}}
    }}
"""

	vals =  {
	            'datarate':int(self.cfg['bandwidth_bps'])/1000,
		    'jitter':int(self.cfg['jitter_us']),
	            'delay':int(self.cfg['latency_us']),
		    'bandwidth_khz':int(self.cfg['bandwidth_khz']),
                    'antennagain':float(self.cfg['antenna_gain_dbi']),
		    'system_noise_figure':float(self.cfg['system_noise_figure']),
                    'pathlossmode':self.cfg['pathloss_mode'],
                    'txpower':float(self.cfg['transmit_power_dbx']),
		    'antennatype':self.cfg['antenna_type'] #add
	        }
	ctemplateformatted = ctemplate.format(**vals)
	return ctemplateformatted

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "RFPipe"
        if verbosity >= 1:
            pretty += "rg%.2f" % float(self.cfg['antenna_gain_dbi'])
        if verbosity >= 2:
            pretty += "-"
            pretty += "%s" % self.cfg['pathloss_mode']
        if verbosity >= 1:
            pretty += "tx%.2f" % float(self.cfg['transmit_power_dbx'])
        if verbosity >= 2:
            pretty += "-"
            pretty += "%s" % self.cfg['antenna_type']
        return pretty

class MacPhyEmane80211(MacPhy):

    def get_custom_cfg_name(self):
        return "emane_ieee80211abg"

    def get_custom_cfg(self):
        ctemplate="""    custom-config {{
	custom-config-id emane_ieee80211abg
	custom-command {{1 11 3 1 1 2 11 10 11 2 10 10 10 10 10 10 9 9 9 10 10 11 4 11 4 4 11 11 10 2 9 9}}
	config {{
	mode={standard_mode}
	enablepromiscuousmode=0
	distance={max_distance_m}
	unicastrate={unicast_rate_mode}
	multicastrate={multicast_rate_mode}
	rtsthreshold=0
	wmmenable=0
	pcrcurveuri=/usr/share/emane/models/ieee80211abg/xml/ieee80211pcr.xml
	flowcontrolenable=0
	flowcontroltokens=10
	queuesize=0:255 1:255 2:255 3:255
	cwmin=0:32 1:32 2:16 3:8
	cwmax=0:1024 1:1024 2:64 3:16
	aifs=0:2 1:2 2:2 3:1
	txop=0:0 1:0 2:0 3:0
	retrylimit=0:3 1:3 2:3 3:3
	antennagain={antennagain}
	antennaazimuth=0.0
	antennaelevation=0.0
	antennaprofileid=1
	antennaprofilemanifesturi=
	antennaprofileenable=0
	bandwidth={bandwidth_khz}
	defaultconnectivitymode=0
	frequency=2.347G
	frequencyofinterest=2.347G
	frequencyofinterestfilterenable=1
	noiseprocessingmode=0
	pathlossmode={pathlossmode}
	subid=1
	systemnoisefigure={system_noise_figure}
	txpower={txpower}
	}}
    }}
"""

	vals =  {
	            'standard_mode':self.cfg['standard_mode'],
		    'max_distance_m':int(self.cfg['max_distance_m']),
		    'unicast_rate_mode':self.cfg['unicast_rate_mode'],
	            'multicast_rate_mode':self.cfg['multicast_rate_mode'],
		    'bandwidth_khz':self.cfg['bandwidth_khz'],
                    'antennagain':float(self.cfg['antenna_gain_dbi']),
		    'system_noise_figure':float(self.cfg['system_noise_figure']),
                    'pathlossmode':self.cfg['pathloss_mode'],
                    'txpower':float(self.cfg['transmit_power_dbx']),
		    'antennatype':self.cfg['antenna_type'] #add
	        }
	ctemplateformatted = ctemplate.format(**vals)
	return ctemplateformatted


    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "802.11"
        if verbosity >= 1:
            pretty += "rg%.2f" % float(self.cfg['antenna_gain_dbi'])
        if verbosity >= 2:
            pretty += "-"
            pretty += "%s" % self.cfg['pathloss_mode']
        if verbosity >= 1:
            pretty += "tx%.2f" % float(self.cfg['transmit_power_dbx'])
        if verbosity >= 2:
            pretty += "-"
            pretty += "%s" % self.cfg['antenna_type']
        return pretty

class MacPhyManager:

    def __init__(self, num_nodes):
        self.num_nodes = num_nodes
    	self.mac_phys = []

    def add_mac_phy(self, mac_phy):
        self.mac_phys = self.mac_phys + [ mac_phy ]

    def get_interfaces(self, node_num):
        ifaces_str = ""
        first = True
        iface_count = 0
        for mac_phy in self.mac_phys:
            if mac_phy.has_interface(node_num):
                if first:
                    ifaces_str += "\t!\n"
                    first = False
                ifaces_str = ifaces_str + mac_phy.get_iface_str(node_num, iface_count)
                iface_count += 1
                ifaces_str += "\t!\n"
        return ifaces_str

    def get_interface_peers(self, node_num):
        ifaces_str = ""
        iface_count = 0
        for mac_phy in self.mac_phys:
            if mac_phy.has_interface(node_num):
                ifaces_str += mac_phy.get_interface_peer(node_num, iface_count)
                iface_count += 1
        return ifaces_str

    def get_macphy_node_configs(self):
        return "\n".join([mac_phy.get_node_cfg() for mac_phy in self.mac_phys])

    def get_link_strings(self):
        link_str = ""
        link_num = 0
        for mac_phy in self.mac_phys:
            node_num = mac_phy.get_node_num()
            for node in mac_phy.get_nodes():
                link_num += 1
                link_str += "\nlink l%d {\n" % link_num
                link_str += "    nodes {n%d n%d}\n" % (node_num, node)
                link_str += "}\n"
        return link_str

    def get_pretty_name(self, verbosity):
        pretty = ""
        for mac_phy in self.mac_phys:
            pretty += mac_phy.get_pretty_name(verbosity)
        return pretty

def get_macphy_module(num_nodes, macphys_cfgs):
    try:
        macphy_manager = MacPhyManager(num_nodes)
        nodes = range(1, num_nodes+1)
        macphy_num = 0
        for macphy_cfg in macphys_cfgs:
            macphy_num = macphy_num + 1
            nodes = macphy_cfg['nodes']
            name = macphy_cfg['name']
            parameters = macphy_cfg['parameters']
            try:
                existing_pretty = parameters['pretty_name']
            except:
                try:
                    parameters['pretty_name'] = macphy_cfg['pretty_name']
                except Exception:
                    None
            if name == 'basic_range':
                macphy = MacPhyBasic(parameters, nodes, num_nodes, macphy_num)
            elif name == 'emane_rfpipe':
                macphy = MacPhyEmaneRFPipe(parameters, nodes, num_nodes, macphy_num)
            elif name == 'emane_ieee80211abg':
                macphy = MacPhyEmane80211(parameters, nodes, num_nodes, macphy_num)
            else:
                raise Exception("Unknown type")
            macphy_manager.add_mac_phy(macphy)
    except Exception, e:
        print e
        return False
    return macphy_manager
