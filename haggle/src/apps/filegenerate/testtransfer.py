import sys

from core import pycore



ct = "/usr/local/bin/haggle"

fg = "/media/5fa210a0-27d9-4e4e-ace0-472d84c4ed9b/research/sri/cbmen-encoders/haggle/src/apps/filegenerate"

print file,ct

session = pycore.Session(persistent=True)
node1 = session.addobj(cls=pycore.nodes.CoreNode, name="n1")
node2 = session.addobj(cls=pycore.nodes.CoreNode, name="n2")
hub1 = session.addobj(cls=pycore.nodes.HubNode, name="hub1")
node1.newnetif(hub1, ["10.0.0.1/24"])
node2.newnetif(hub1, ["10.0.0.2/24"])


node1.cmd(["/media/5fa210a0-27d9-4e4e-ace0-472d84c4ed9b/research/sri/cbmen-encoders/haggle/src/apps/filegenerate/run1.sh"],wait=True)
#node1.cmd([ct,"-f","-d"],wait=False)

node2.cmd(["/media/5fa210a0-27d9-4e4e-ace0-472d84c4ed9b/research/sri/cbmen-encoders/haggle/src/apps/filegenerate/run2.sh"],wait=True)
#node2.cmd([ct,"-f","-d"],wait=False)

#session.shutdown()