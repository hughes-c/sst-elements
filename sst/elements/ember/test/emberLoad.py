
import sys,getopt

import sst
from sst.merlin import *

import loadInfo
from loadInfo import *

import networkConfig 
from networkConfig import *

loadFile = ""
workList = []
numCores = 1
debug    = 0
topology = ""
shape    = ""
loading  = 0
radix    = 0
emberVerbose = 0

motifDefaults = { 
'cmd' : "",
'printStats' : 0, 
'api': "HadesMP",
'spyplotmode': 0 
}

netBW = "4GB/s"
netPktSize="2048B"
netFlitSize="8B"
netBufSize="14KB"


if 1 == len(sys.argv) :
    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Init"
    workList.append( motif )

    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Sweep3D nx=30 ny=30 nz=30 computetime=140 pex=4 pey=16 pez=0 kba=10"
    motif['spyplotmode'] = 0
    workList.append( motif )

    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Fini"
    workList.append( motif )

    topology = "torus"
    shape    = "4x4x4"

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
					"radix=","loading=","debug=",
					"numCores=","loadFile=","cmdLine=","printStats=",
					"emberVerbose=","netBW=","netPktSize=","netFlitSize="])

except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        shape = a
    elif o in ("--numCores"):
        numCores = a
    elif o in ("--debug"):
        debug = a
    elif o in ("--loadFile"):
        loadFile = a
    elif o in ("--cmdLine"):
    	motif = dict.copy(motifDefaults)
    	motif['cmd'] = a 
    	workList.append( motif )
    elif o in ("--topo"):
        topology = a
    elif o in ("--radix"):
        radix = a
    elif o in ("--loading"):
        loading = a
    elif o in ("--printStats"):
        printStats = a
    elif o in ("--emberVerbose"):
        emberVerbose = a
    elif o in ("--netBW"):
        netBW = a
    elif o in ("--netFlitSize"):
        netFlitSize = a
    elif o in ("--netPktSize"):
        netPktSize = a
    else:
        assert False, "unhandle option" 


if "" == topology:
	sys.exit("What topo? [torus|fattree]")

if "torus" == topology:
	if "" == shape:
		sys.exit("What torus shape? (e.x. 4, 2x2, 4x4x8)")
	topoInfo = TorusInfo(shape)
	topo = topoTorus()
	print "network: topology=torus shape={0}".format(shape)

elif "fattree" == topology:
	if "" == shape: # use shape if defined, otherwise use radix as legacy mode
		if 0 == radix: 
			sys.exit("Must either specify shape or radix/loading.")
		if 0 == loading:
			sys.exit("Must either specify shape or radix/loading.")
	topoInfo = FattreeInfo(radix,loading,shape)
	topo = topoFatTree()
	print "network: topology=fattree radix={0} loading={1}".format(radix,loading)
else:
	sys.exit("how did we get here")

print "network: BW={0} pktSize={1} flitSize={2}".format(netBW,netPktSize,netFlitSize)

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = netBW 
sst.merlin._params["xbar_bw"] = netBW 
sst.merlin._params["flit_size"] = netFlitSize
sst.merlin._params["input_latency"] = "50ns"
sst.merlin._params["output_latency"] = "50ns"
sst.merlin._params["input_buf_size"] = netBufSize 
sst.merlin._params["output_buf_size"] = netBufSize 

sst.merlin._params.update( topoInfo.getNetworkParams() )

_nicParams = { 
		"debug" : debug,
		"verboseLevel": 1,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin." + topology,
		"packetSize" : netPktSize,
		"link_bw" : netBW,
		"buffer_size" : netBufSize,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 50,
	}

_emberParams = {
		"os.module"    : "firefly.hades",
		"os.name"      : "hermesParams",
		"api.0.module" : "firefly.hadesMP",
		"verbose" : emberVerbose,
	}

_hermesParams = {
		"hermesParams.debug" : debug,
		"hermesParams.verboseLevel" : 1,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : debug,
		"hermesParams.nicParams.debugLevel" : 1 ,
		"hermesParams.policy" : "adjacent",
		"hermesParams.functionSM.defaultEnterLatency" : 30000,
		"hermesParams.functionSM.defaultReturnLatency" : 30000,
		"hermesParams.functionSM.defaultDebug" : debug,
		"hermesParams.functionSM.defaultVerbose" : 2,
		"hermesParams.ctrlMsg.debug" : debug,
		"hermesParams.ctrlMsg.verboseLevel" : 2,
		"hermesParams.ctrlMsg.shortMsgLength" : 12000,
		"hermesParams.ctrlMsg.matchDelay_ns" : 150,

        "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-:130ns",

        "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-:100ns",

        "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
        "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-:344ps",

        "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
        "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-:344ps",

		"hermesParams.ctrlMsg.txNicDelay_ns" : 0,
		"hermesParams.ctrlMsg.rxNicDelay_ns" : 0,
		"hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 0,
		"hermesParams.ctrlMsg.sendAckDelay_ns" : 0,
		"hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3000,
		"hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 100,
		"hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
		"hermesParams.loadMap.0.start" : 0,
		"hermesParams.loadMap.0.len" : 2,
	}


epParams = {} 
epParams.update(_emberParams)
epParams.update(_hermesParams)

loadInfo = LoadInfo( _nicParams, epParams, topoInfo.getNumNodes(), numCores )

if len(loadFile) > 0:
	if len(workList) > 0:
		sys.exit("Error: can't specify both loadFile and cmdLine");

	loadInfo.initFile( motifDefaults, loadFile)
else:
	if len(workList) > 0:
		if len(loadFile) > 0:
			sys.exit("Error: can't specify both loadFile and cmdLine");

		loadInfo.initWork( workList )
	else:
		sys.exit("Error: need a loadFile or cmdLine")

topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
