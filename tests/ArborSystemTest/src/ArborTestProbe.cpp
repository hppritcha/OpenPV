/*
 * ArborTestProbe.cpp
 *
 *  Created on: Sep 6, 2011
 *      Author: kpeterson
 */

#include "ArborTestProbe.hpp"
#include <include/pv_arch.h> 
#include <layers/HyPerLayer.hpp>
#include <string.h>
#include <utils/PVLog.hpp>

namespace PV {

ArborTestProbe::ArborTestProbe(const char * probeName, HyPerCol * hc)
: StatsProbe()
{
   initArborTestProbe_base();
   initArborTestProbe(probeName, hc);
}

ArborTestProbe::~ArborTestProbe() {}

int ArborTestProbe::initArborTestProbe_base() { return PV_SUCCESS;}

int ArborTestProbe::initArborTestProbe(const char * probeName, HyPerCol * hc) {
   return initStatsProbe(probeName, hc);
}

void ArborTestProbe::ioParam_buffer(enum ParamsIOFlag ioFlag) {
   if (ioFlag==PARAMS_IO_READ) {
      type = BufActivity;
      PVParams * params = getParams();
      if (params->present(name, "buffer")) {
         params->handleUnnecessaryStringParameter(name, "buffer");
         char const * buffer = params->stringValue(name, "buffer");
         pvErrorIf(!(buffer != NULL), "Test failed.\n");
         char * bufferlc = strdup(buffer);
         for (int c=0; c<(int) strlen(bufferlc); c++) { bufferlc[c] = tolower(bufferlc[c]); }
         if (strcmp(bufferlc, "a")!=0 && strcmp(bufferlc, "activity")!=0) {
            if (getCommunicator()->commRank()==0) {
               pvErrorNoExit().printf("   Value \"%s\" is inconsistent with correct value \"a\" or \"activity\".  Exiting.\n", buffer);
            }
            MPI_Barrier(getCommunicator()->communicator());
            exit(EXIT_FAILURE);
         }
         free(bufferlc);

      }
   }
}

int ArborTestProbe::outputState(double timed)
{
   int status = StatsProbe::outputState(timed);
   Communicator * icComm = getTargetLayer()->getCommunicator();
   const int rcvProc = 0;
   if( icComm->commRank() != rcvProc ) {
      return 0;
   }
   for(int b = 0; b < mBatchWidth; b++){
      if(timed==1.0f){
         pvErrorIf(!((avg[b]>0.2499)&&(avg[b]<0.2501)), "Test failed.\n");
      }
      else if(timed==2.0f){
         pvErrorIf(!((avg[b]>0.4999)&&(avg[b]<0.5001)), "Test failed.\n");
      }
      else if(timed==3.0f){
         pvErrorIf(!((avg[b]>0.7499)&&(avg[b]<0.7501)), "Test failed.\n");
      }
      else if(timed>3.0f){
         pvErrorIf(!((fMin[b]>0.9999)&&(fMin[b]<1.001)), "Test failed.\n");
         pvErrorIf(!((fMax[b]>0.9999)&&(fMax[b]<1.001)), "Test failed.\n");
         pvErrorIf(!((avg[b]>0.9999)&&(avg[b]<1.001)), "Test failed.\n");
      }
   }

   return status;
}

} /* namespace PV */
