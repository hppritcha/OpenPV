import os, sys
lib_path = os.path.abspath("/home/slundquist/workspace/PetaVision/plab/")
sys.path.append(lib_path)
from plotRecon import plotRecon
from plotReconError import plotReconError

#For plotting
#import matplotlib.pyplot as plt

outputDir = "/nh/compneuro/Data/Depth/LCA/stack_v2_nowhite/"
skipFrames = 20 #Only print every 20th frame
doPlotRecon = True
doPlotErr = True
errShowPlots = False
layers = [
   "a2_LeftRescale",
   "a4_LeftReconS2",
   "a5_LeftReconS4",
   "a6_LeftReconS8",
   "a7_LeftReconAll",
   "a10_RightRescale",
   "a12_RightReconS2",
   "a13_RightReconS4",
   "a14_RightReconS8",
   "a15_RightReconAll",
   "a23_DepthRescale",
   "a25_DepthRecon",
   "a32_V2V1S2LeftRecon",
   "a33_V2V1S4LeftRecon",
   "a34_V2V1S8LeftRecon",
   "a35_V2V1AllLeftRecon",
   "a36_V2V1S2RightRecon",
   "a37_V2V1S4RightRecon",
   "a38_V2V1S8RightRecon",
   "a39_V2V1AllRightRecon"
   ]
#Layers for constructing recon error
preErrLayers = [
   "a2_LeftRescale",
   "a2_LeftRescale",
   "a2_LeftRescale",
   "a7_LeftReconAll",
   "a7_LeftReconAll",
   "a7_LeftReconAll",
]

postErrLayers = [
   "a4_LeftReconS2",
   "a5_LeftReconS4",
   "a6_LeftReconS8",
   "a4_LeftReconS2",
   "a5_LeftReconS4",
   "a6_LeftReconS8",
]

gtLayers = None
#gtLayers = [
#   #"a25_DepthRescale",
#   #"a25_DepthRescale",
#   #"a25_DepthRescale",
#   "a25_DepthRescale",
#   "a25_DepthRescale",
#   "a25_DepthRescale",
#]

preToPostScale = [
   .0294,
   .0294,
   .0294,
   1,
   1,
   1,
]


if(doPlotRecon):
   print "Plotting reconstructions"
   plotRecon(layers, outputDir, skipFrames)

if(doPlotErr):
   print "Plotting reconstruction error"
   plotReconError(preErrLayers, postErrLayers, preToPostScale, outputDir, errShowPlots, skipFrames, gtLayers)
