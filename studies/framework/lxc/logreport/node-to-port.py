# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

  def checkConfiguration(self) :
    configRoot = self.testRoot+"/"+CONFIGD
    configs = re.compile("(.+)").findall(lxcmd("ls "+configRoot))
    for fname in configs :
      xml = minidom.parse(configRoot+"/"+fname)
      for eaPair in criticalConfig:
        e = eaPair[0]
        a = eaPair[1]
        v = self.getValueFromXml(xml, e,a)
        if v != None :
          self.config.append([e+"."+a,v])
      break

  def getValueFromXml(self, xml, element, attr) :
    ret = xml.getElementsByTagName(element)
    for e in ret:
      return e.attributes[attr].value
