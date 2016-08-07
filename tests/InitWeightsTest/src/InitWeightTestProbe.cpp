/*
 * InitWeightTestProbe.cpp
 *
 *  Created on: Sep 6, 2011
 *      Author: kpeterson
 */

#include "InitWeightTestProbe.hpp"
#include <include/pv_arch.h>
#include <layers/HyPerLayer.hpp>
#include <string.h>
#include <utils/PVLog.hpp>

namespace PV {

InitWeightTestProbe::InitWeightTestProbe(const char * probeName, HyPerCol * hc)
: StatsProbe()
{
   initInitWeightTestProbe(probeName, hc);
}

int InitWeightTestProbe::initInitWeightTestProbe_base() { return PV_SUCCESS; }

int InitWeightTestProbe::initInitWeightTestProbe(const char * probeName, HyPerCol * hc) {
   return initStatsProbe(probeName, hc);
}

void InitWeightTestProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   requireType(BufActivity);
}


int InitWeightTestProbe::outputState(double timed)
{
   int status = StatsProbe::outputState(timed);
   Communicator * icComm = getTargetLayer()->getCommunicator();
   const int rcvProc = 0;
   if( icComm->commRank() != rcvProc ) {
      return 0;
   }
   for(int b = 0; b < mBatchWidth; b++){
      if(timed>2.0f){
         pvErrorIf(!((fMin[b]>-0.001)&&(fMin[b]<0.001)), "Test failed.\n");
         pvErrorIf(!((fMax[b]>-0.001)&&(fMax[b]<0.001)), "Test failed.\n");
         pvErrorIf(!((avg[b]>-0.001)&&(avg[b]<0.001)), "Test failed.\n");
      }
   }

   return status;
}

} /* namespace PV */
