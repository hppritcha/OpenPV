/*
 * GPUSystemTestProbe.cpp
 * Author: slundquist
 */

#include "GPUSystemTestProbe.hpp"
#include <include/pv_arch.h>
#include <layers/HyPerLayer.hpp>
#include <assert.h>
#include <string.h>

namespace PV {
GPUSystemTestProbe::GPUSystemTestProbe(const char * probeName, HyPerCol * hc)
   : StatsProbe()
{
   initGPUSystemTestProbe_base();
   initGPUSystemTestProbe(probeName, hc);
}

int GPUSystemTestProbe::initGPUSystemTestProbe_base() { return PV_SUCCESS; }

int GPUSystemTestProbe::initGPUSystemTestProbe(const char * probeName, HyPerCol * hc) {
   return initStatsProbe(probeName, hc);
}

void GPUSystemTestProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   requireType(BufActivity);
}

//2 tests: max difference can be 5e-4, max std is 5e-5
int GPUSystemTestProbe::outputState(double timed){
   int status = StatsProbe::outputState(timed);
   const PVLayerLoc * loc = getTargetLayer()->getLayerLoc();
   int numExtNeurons = getTargetLayer()->getNumExtendedAllBatches();
   const pvdata_t * A = getTargetLayer()->getLayerData();
   float sumsq = 0;
   for (int i = 0; i < numExtNeurons; i++){
      assert(fabs(A[i]) < 5e-4);
   }
   for(int b = 0; b < loc->nbatch; b++){
      //For max std of 5e-5
      assert(sigma[b] <= 5e-5);
   }

   return status;
}

BaseObject * createGPUSystemTestProbe(char const * probeName, HyPerCol * hc) {
   return hc ? new GPUSystemTestProbe(probeName, hc) : NULL;
}

} // end namespace PV
