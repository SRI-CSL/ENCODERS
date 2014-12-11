# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Contains configuration objects used by other code.

"""

import os
import sys
from clitools import Console

openSSLConfigFileData = """
HOME            = .
RANDFILE        = $ENV::HOME/.rnd

[ ca ]
default_ca  = cbmen_CA      # The default ca section

[ cbmen_CA ]

dir     = /tmp/cbmenCA      # Where everything is kept
certs       = $dir      # Where the issued certs are kept
crl_dir     = $dir      # Where the issued crl are kept
database    = $dir/index.txt    # database index file.
unique_subject  = no            # Set to 'no' to allow creation of
                    # several ctificates with same subject.
new_certs_dir   = $dir      # default place for new certs.

certificate = $dir/cacert.pem   # The CA certificate
serial      = $dir/serial       # The current serial number
crlnumber   = $dir/crlnumber    # the current crl number
                    # must be commented out to leave a V1 CRL
crl     = $dir/crl.pem      # The current CRL
private_key = $dir/cakey.pem # The private key
RANDFILE    = $dir/.rand    # private random number file

x509_extensions = usr_cert      # The extentions to add to the cert

# Comment out the following two lines for the "traditional"
# (and highly broken) format.
name_opt    = ca_default        # Subject Name options
cert_opt    = ca_default        # Certificate field options


default_days    = 3650          # how long to certify for
default_crl_days= 30            # how long before next CRL
default_md  = default       # use public key default MD
preserve    = no            # keep passed DN ordering

policy      = policy_match

[ policy_match ]
countryName     = optional
stateOrProvinceName = optional
organizationName    = optional
organizationalUnitName  = optional
commonName      = supplied
emailAddress        = optional

[ usr_cert ]
basicConstraints=CA:FALSE
nsComment           = "OpenSSL Generated Certificate"
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer

[ crl_ext ]
authorityKeyIdentifier=keyid:always
"""

class BaseConfig(object):

    def __init__(self):
        pass

    def update(self, dic, report=False):
        myvars = vars(self)
        for key, val in dic.iteritems():
            if key in myvars:
                setattr(self, key, val)
                if report:
                    Console.info('Setting %s to %s' % (key, val))
            else:
                Console.warning('Unknown option %s, ignoring!' % key)

    def toDict(self):
        dic = {}
        myvars = vars(self)
        for var in myvars:
            dic[var] = getattr(self, var)
        return dic

    def __repr__(self):
        output = ''
        output += '--------------------------------------------------------------\n'
        output += '%s:\n' % self.__class__.__name__
        output += '--------------------------------------------------------------\n'
        for key in vars(self):
            output += '%s: %s\n' % (key, getattr(self, key))
        output += '--------------------------------------------------------------\n'
        return output

class SecurityConfig(BaseConfig):

    def __init__(self):
        super(SecurityConfig, self).__init__()

        # Security parameters
        self.signatureChaining = False
        self.securityLevel = 'LOW'
        self.encryptFilePayload = False
        self.certificateSigningRequestDelay = 1
        self.certificateSigningRequestRetries = -1
        self.certificateSigningFirstRequestDelay = 2
        self.attributeRequestDelay = 60
        self.maxOutstandingRequests = 40
        self.charmPersistenceData = 'eJyrVkosLcnIL8osqYwvzlayUqiu1VFQKi1OLULiIpQUwMXSC0AspXQgqWRolWucka2dXRluEBhl6OsXVKJvnGeWExpRFmpqFOZcHOGmnWaSkh/pmZFi6lPkX55RZmga5lVhbFLsGOCcG2WhbeCXVJalb14QGVYVGVAekVJSGeGd4ufoaKtUWwsA7yI01Q=='
        self.signNodeDescriptions = False
        self.tempFilePath = None
        self.rsaKeyLength = 512
        self.compositeSecurityDataRequests = False

        # openssl commands
        self.openSSLTmpCertFolder = "/tmp/certs"
        self.openSSLCAFolder = "/tmp/cbmenCA" # change config file data if this is changed
        self.openSSLConfigFile = "/tmp/openssl.cnf"
        self.openSSLConfigFileData = openSSLConfigFileData
        self.openSSLInitCAFolderCommand = """rm -rf {caFolder}; mkdir {caFolder}; echo "01" >> {caFolder}/serial; echo "{seed}" >> {caFolder}/.rand; touch {caFolder}/index.txt; ln -s {certFolder}/{id}.pem {caFolder}/cakey.pem; ln -s {certFolder}/{id}.crt {caFolder}/cacert.pem"""
        self.openSSLGenerateKeyPairAndSelfSignedCertCommand = """/usr/bin/openssl req -x509 -nodes -days 365 -newkey rsa:{size} -keyout {certFolder}/{id}.pem -out {certFolder}/{id}.crt -subj '/CN={id}'"""
        self.openSSLGenerateKeyPairAndSelfSignedCertCommandList = ['/usr/bin/openssl', 'req', '-x509', '-nodes', '-days', '365', '-newkey', 'rsa:{size}', '-keyout', '{certFolder}/{id}.pem', '-out', '{certFolder}/{id}.crt', '-subj', "/CN={id}"]
        self.openSSLGetPublicKeyCommand = """/usr/bin/openssl rsa -in {certFolder}/{id}.pem -pubout >{certFolder}/{id}.pub"""
        self.openSSLGetPublicKeyCommandList = ['/usr/bin/openssl', 'rsa', '-in', '{certFolder}/{id}.pem', '-pubout']
        self.openSSLGenerateCertificateSigningRequest = """/usr/bin/openssl req -key {certFolder}/{id}.pem -new -subj '/CN={id}' -out {certFolder}/{id}.req"""
        self.openSSLGenerateCertificateSigningRequestList = ['/usr/bin/openssl', 'req', '-key', '{certFolder}/{id}.pem', '-new', '-subj', "/CN={id}", '-out', '{certFolder}/{id}.req']
        self.openSSLCASignCertificateCommand = """/usr/bin/openssl ca -config {configFile} -batch -notext -out {certFolder}/{subject}_{issuer}.crt -infiles {certFolder}/{subject}.req"""
        self.openSSLCASignCertificateCommandList = ['/usr/bin/openssl', 'ca', '-config', '{configFile}', '-batch', '-notext', '-out', '{certFolder}/{subject}_{issuer}.crt', '-infiles', '{certFolder}/{subject}.req']

class TestFrameworkConfig(SecurityConfig):

    def __init__(self):
        super(TestFrameworkConfig, self).__init__()

        # Basics - These will be autodiscovered
        self.whoami = None
        self.cbmenFolder = None
        self.baseHaggleConfigDirectory = None
        self.haggleConfigLocation = None

        # Programs we need
        self.adbPath = 'adb'

        # Config files we need
        self.baseHaggleConfigFile = "config-flood-direct-thirdpartynd-noshortcut.xml"

        # General delay parameters - android auto discover changes these
        self.subscribeTimeout = 5
        self.haggleStopDelay = 1
        self.haggleBootDelay = 1
        self.publishDelay = 1
        self.deleteHaggleDBDelay = 1
        self.dynamicConfigurationDelay = 1
        self.sleepFactor = 1
        self.deviceRebootDelay = 7
        self.deviceInstallDelay = 2
        self.exchangeDelay = 3

        # Core options
        self.coreHaggle = '/usr/local/bin/haggle'
        self.coreHaggleOptions = '-f'
        self.coreHaggleTest = '/usr/local/bin/haggletest'
        self.coreHaggleDataDirectory = None

        # Core parameters
        self.coredLocation = '/usr/sbin'
        self.coreLinearTopology = False
        self.coreWLANRange = 2000
        self.coreRangeMargin = 10
        self.coreEdgeOffset = 1
        self.coreMinX = 1
        self.coreMinY = 1
        self.coreMaxX = 800
        self.coreMaxY = 800
        self.coreWLANX = 250
        self.coreWLANY = 250
        self.coreExternalHaggleNodeIDCommand = """ifconfig -a | awk '/HWaddr/ {print substr($5,0)}' | python -c 'import sys; sys.stdout.write("\\x00" * 6); sys.stdout.write("".join(chr(int(n, 16)) for n in sys.stdin.read().split(":")))' | sha1sum | awk '{print $0}'"""
        self.coreHagglePIDCommandList = ['/bin/bash', '-c', "ps h -C haggle | grep haggle | awk '{print $1;}'"]
        self.coreLimitCPUCommandList = ['/usr/bin/limitcpu', '-b', '--quiet', '--pid', '{pid}', '--limit', '{cpulimit}']
        self.coreCleanupLimitCPUCommandList = ['/usr/bin/killall', '-9', 'limitcpu']

        # Device options
        self.deviceHaggleAPK = None
        self.deviceHaggleTest = '/data/haggletest'
        self.deviceHaggleApp = 'org.haggle.kernel'
        self.deviceHaggleIntent = 'am startservice -a android.intent.action.MAIN -n org.haggle.kernel/org.haggle.kernel.Haggle'
        self.deviceHaggleDataDirectory = '/data/data/%s/files' % self.deviceHaggleApp
        self.deviceHaggleLogFile = '%s/haggle.log' % self.deviceHaggleDataDirectory
        self.deviceHaggleConfigFile = '%s/config.xml' % self.deviceHaggleDataDirectory
        self.deviceHaggleDBFile = '%s/haggle.db' % self.deviceHaggleDataDirectory

        # Device parameters
        self.deviceMeshBaseIP = '192.168.0'
        self.deviceGetADBDeviceListCommand = "%s devices | awk 'NR > 1 && length > 0 {print $1;}'" % self.adbPath
        self.deviceGetHaggleUIDCommand = "shell dumpsys package org.haggle.kernel | grep userId | awk '{split($1,a,\"=\"); print a[2];}'"

        # Microbenchmark parameters
        self.microBenchmarkNumFiles = 100
        self.microBenchmarkFileSize = 1024
        self.microBenchmarkCPULimit = 100
        self.microBenchmarkPublishDelay = 1
        self.microBenchmarkNumAttributes = 8
        self.microBenchmarkRSAKeyLength = 512

        # Misc options
        self.testEnvironment = 'core'
        self.testContentSize = 100
        self.logDirectory = 'log'
        self.verbose = True
        self.tmpConfigLocation = '/tmp/config.xml'
        self.tmpLogLocation = '/tmp/haggle.log'
        self.tmpDBLocation = '/tmp/haggle.db'

    def _autoDiscover(self):
        self.whoami = os.getlogin()
        Console.info('Discovered whoami: %s' % self.whoami)

        self.cbmenFolder = os.path.abspath('../../')
        if os.path.exists(os.path.join(self.cbmenFolder, 'ccb')) and \
            os.path.exists(os.path.join(self.cbmenFolder, 'charm')) and \
            os.path.exists(os.path.join(self.cbmenFolder, 'haggle')):
            Console.info('Discovered CBMEN folder: %s' % self.cbmenFolder)
            self.baseHaggleConfigDirectory = os.path.join(self.cbmenFolder, 'haggle', 'resources')
            self.haggleConfigLocation = os.path.join(self.baseHaggleConfigDirectory, self.baseHaggleConfigFile)
            Console.info('Setting baseHaggleConfigDirectory to %s' % self.baseHaggleConfigDirectory)
            Console.info('Setting haggleConfigLocation to %s' % self.haggleConfigLocation)
        elif os.path.exists(self.baseHaggleConfigFile):
            Console.info('Setting haggleConfigLocation to %s' % self.baseHaggleConfigFile)
            self.baseHaggleConfigDirectory = os.path.abspath('.')
            self.haggleConfigLocation = self.baseHaggleConfigFile


    def autoDiscoverCore(self):
        self._autoDiscover()
        self.coreHaggleDataDirectory = '/tmp'

    def autoDiscoverAndroid(self):
        self._autoDiscover()
        self.deviceHaggleAPK = os.path.join(self.cbmenFolder, 'haggle', 'bin', 'Haggle-debug.apk')

        if not os.path.exists(self.deviceHaggleAPK):
            Console.warning('Haggle APK not found at %s, please make sure it is present or configured in config file!' % self.deviceHaggleAPK)
        else:
            Console.info('Set Haggle APK to %s' % self.deviceHaggleAPK)

        self.sleepFactor = 6
        self.subscribeTimeout = 2
        self.dynamicConfigurationDelay = 2
        self.publishDelay = 2
        self.certificateSigningFirstRequestDelay = 5

    def validate(self, environmentClass):
        toValidate = ['whoami', 'haggleConfigLocation']
        if environmentClass.__name__ == 'CoreTestEnvironment' or environmentClass.__name__ == 'MockCoreTestEnvironment':
            toValidate.append('coreHaggleDataDirectory')
        elif environmentClass.__name__ == 'AndroidTestEnvironment' or environmentClass.__name__ == 'MockAndroidTestEnvironment':
            toValidate.append('deviceHaggleAPK')
        else:
            raise Exception('Invalid environment class! %s' % environmentClass.__name__)
        for param in toValidate:
            if getattr(self, param) is None:
                Console.fail('Config param %s was not auto-discovered and not set in config file!' % param)
                sys.exit(1)
