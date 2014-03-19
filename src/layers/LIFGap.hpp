/*
 * LIFGap.hpp
 *
 *  Created on: Jul 29, 2011
 *      Author: garkenyon
 */

#ifndef LIFGAP_HPP_
#define LIFGAP_HPP_

#include "LIF.hpp"

#define NUM_LIFGAP_EVENTS   1 + NUM_LIF_EVENTS  // ???
//#define EV_LIF_GSYN_GAP     NUM_LIF_EVENTS + 1
#define EV_LIFGAP_GSYN_GAP     3
//#define EV_LIFGAP_ACTIVITY  4


namespace PV {

class LIFGap: public PV::LIF {
public:
   LIFGap(const char* name, HyPerCol * hc);
   LIFGap(const char* name, HyPerCol * hc, PVLayerType type);
   virtual ~LIFGap();

   void addGapStrength(float gap_strength){sumGap += gap_strength;}
   int virtual updateStateOpenCL(double time, double dt);
#ifdef OBSOLETE // Marked obsolete July 25, 2013.  recvSynapticInput is now called by recvAllSynapticInput, called by HyPerCol, so deliver andtriggerReceive aren't needed.
   int virtual triggerReceive(InterColComm* comm);
#endif // OBSOLETE
   int virtual updateState(double time, double dt);

#ifdef OBSOLETE // Marked obsolete May 15, 2013.  G_Gap wasn't being used (GSyn[3] isn't filtered into a conductance) so the checkpointed G_Gap was all zeroes.
   virtual int checkpointRead(const char * cpDir, double * timef);
   virtual int checkpointWrite(const char * cpDir);
#endif //OBSOLETE

protected:

   LIFGap();
   int initialize(const char * name, HyPerCol * hc, PVLayerType type, const char * kernel_name);
   virtual int allocateConductances(int num_channels);

   // pvdata_t * G_Gap; // Commented out May 15, 2013
   pvdata_t sumGap;

#ifdef PV_USE_OPENCL
   virtual int initializeThreadBuffers(const char * kernelName);
   virtual int initializeThreadKernels(const char * kernelName);

   // OpenCL buffers
   //
   CLBuffer * clG_Gap;
   CLBuffer * clGSynGap;

   virtual int getEVGSynGap() {return EV_LIFGAP_GSYN_GAP;}
   //virtual int getEVActivity() {return EV_LIFGAP_ACTIVITY;}
   virtual inline int getGSynEvent(ChannelType ch) {
      if(LIF::getGSynEvent(ch)>=0) return LIF::getGSynEvent(ch);
      if(ch==CHANNEL_GAP) return getEVGSynGap();
      return -1;
   }
   virtual int getNumCLEvents(){return NUM_LIFGAP_EVENTS;}
   virtual const char * getKernelName() {return "LIFGap_update_state";}
#endif

private:
   int initialize_base();

};

} /* namespace PV */
#endif /* LIFGAP_HPP_ */
