/*
 * InitBIDSLateralParams.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: bnowers
 */

#include "InitBIDSLateralParams.hpp"

namespace PV {

InitBIDSLateralParams::InitBIDSLateralParams()
{
   initialize_base();
}
InitBIDSLateralParams::InitBIDSLateralParams(HyPerConn * parentConn)
                     : InitWeightsParams() {
   initialize_base();
   initialize(parentConn);
}

InitBIDSLateralParams::~InitBIDSLateralParams()
{
   free(falloffType); falloffType = NULL;
   free(jitterSource); jitterSource = NULL;
}

int InitBIDSLateralParams::initialize_base() {

   // default values
   strength = 1.0f;
   coords = NULL;
   jitterSource = NULL;
   falloffType = NULL;

   return 1;
}

int InitBIDSLateralParams::initialize(HyPerConn * parentConn) {
   return InitWeightsParams::initialize(parentConn);

}

int InitBIDSLateralParams::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = InitWeightsParams::ioParamsFillGroup(ioFlag);
   // old if condition failed to account for connections between oriented to non-oriented cells

   return status;
}

void InitBIDSLateralParams::ioParam_strength(enum ParamsIOFlag ioFlag) {
   parentConn->ioParam_strength(ioFlag, &strength, true/*warnIfAbsent*/);
}

void InitBIDSLateralParams::ioParam_falloffType(enum ParamsIOFlag ioFlag) {
   parent->ioParamString(ioFlag, name, "falloffType", &falloffType, falloffType);
   if (ioFlag==PARAMS_IO_READ && falloffType==NULL) {
      if (parent->columnId()==0) {
         fprintf(stderr, "%s \"%s\": falloffType must be set.  Allowable values are \"Gaussian\", \"radSquared\", or \"Log\".\n",
               parent->parameters()->groupKeywordFromName(name), name);
      }
   }
}

void InitBIDSLateralParams::ioParam_lateralRadius(enum ParamsIOFlag ioFlag) {
   parent->ioParamValue(ioFlag, name, "lateralRadius", &lateralRadius, lateralRadius);
}

void InitBIDSLateralParams::ioParam_jitterSource(enum ParamsIOFlag ioFlag) {
   parent->ioParamStringRequired(ioFlag, name, "jitterSource", &jitterSource);
}

int InitBIDSLateralParams::communicateParamsInfo() {
   int status = InitWeightsParams::communicateParamsInfo();
   BIDSMovieCloneMap * post = dynamic_cast<BIDSMovieCloneMap *>(parent->getLayerFromName(jitterSource));
   assert (post != NULL);
   coords = post->getCoords();

   HyPerLayer * jitter_layer = parent->getLayerFromName(jitterSource);
   BIDSMovieCloneMap * bids_layer = dynamic_cast<BIDSMovieCloneMap *>(jitter_layer);
   if (bids_layer == NULL) {
      if (parent->columnId()==0) {
         fprintf(stderr, "%s \"%s\" error: jitterSource \"%s\" is not a BIDSMovieCloneMap.\n",
               parent->parameters()->groupKeywordFromName(name), name, jitterSource);
      }
      MPI_Barrier(parent->icCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   jitter = bids_layer->getJitter();

   return PV_SUCCESS;
}

void InitBIDSLateralParams::calcOtherParams(int patchIndex) {
   this->getcheckdimensionsandstrides();

   const int kfPre_tmp = this->kernelIndexCalculations(patchIndex);
}

} /* namespace PV */
