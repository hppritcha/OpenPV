/*
 * KernelConn.hpp
 *
 *  Created on: Aug 6, 2009
 *      Author: gkenyon
 */

#ifndef KERNELCONN_HPP_
#define KERNELCONN_HPP_

#include "HyPerConn.hpp"
#ifdef PV_USE_MPI
#  include <mpi.h>
#else
#  include "../include/mpi_stubs.h"
#endif // PV_USE_MPI

namespace PV {

class KernelConn: public HyPerConn {

public:

   KernelConn();
   virtual ~KernelConn();

   KernelConn(const char * name, HyPerCol * hc, HyPerLayer * pre, HyPerLayer * post,
              ChannelType channel, const char * filename = NULL, InitWeights *weightInit = NULL);
   virtual int numDataPatches();

   virtual float minWeight(int axonId = 0);
   virtual float maxWeight(int axonId = 0);

   virtual int checkNormalizeArbor(PVPatch ** patches, int numPatches, int arborId);
   virtual int normalizeWeights(PVPatch ** patches, int numPatches, int arborId);
   virtual int symmetrizeWeights(PVPatch ** patches, int numPatches, int arborId);

   PVPatch * getKernelPatch(int axonId, int kernelIndex)   {return kernelPatches[axonId][kernelIndex];}
   PVPatch ** getKernelPatches(int axonId)   {return kernelPatches[axonId];}
   inline void setKernelPatches(PVPatch** newKernelPatch, int axonId) {kernelPatches[axonId]=newKernelPatch;}
   inline void setKernelPatch(PVPatch* newKernelPatch, int axonId, int kernelIndex) {kernelPatches[axonId][kernelIndex]=newKernelPatch;}
   virtual int writeWeights(float time, bool last=false);
   virtual int writeWeights(const char * filename);
   // virtual int writeWeights(PVPatch *** patches, int numPatches, const char * filename, float timef, bool last);
   inline PVPatch *** getAllKernelPatches() {return kernelPatches;}
   inline const pvdata_t * get_dKernelData(int axonId, int kernelIndex) {if( dKernelPatches && axonId>=0 && axonId<numAxonalArborLists && kernelIndex>=0 && kernelIndex<numDataPatches()) { return dKernelPatches[axonId][kernelIndex]->data;} else return NULL;}

   virtual int shrinkPatches(int arborId);

   bool getPlasticityFlag() {return plasticityFlag;}
   float getWeightUpdatePeriod() {return weightUpdatePeriod;}
   float getWeightUpdateTime() {return weightUpdateTime;}
   float getLastUpdateTime() {return lastUpdateTime;}

   virtual int correctPIndex(int patchIndex);

   virtual int checkpointWrite();
   virtual int checkpointRead(float *timef);

   virtual int kernelIndexToPatchIndex(int kernelIndex, int * kxPatchIndex = NULL,
         int * kyPatchIndex = NULL, int * kfPatchIndex = NULL);

   virtual int patchIndexToKernelIndex(int patchIndex, int * kxKernelIndex = NULL,
         int * kyKernelIndex = NULL, int * kfKernelIndex = NULL);

protected:
//   bool plasticityFlag;
   float weightUpdatePeriod;
   float weightUpdateTime;
   float lastUpdateTime;
   bool symmetrizeWeightsFlag;
   PVPatch ** tmpPatch;  // stores most recently allocated PVPatch **, set to NULL after assignment


private:
   //made private to control use and now 3D to allow different Kernel patches
   //for each axon:
   PVPatch *** kernelPatches;   // list of kernel patches


protected:
   PVPatch *** dKernelPatches;   // list of dKernel patches for storing changes in kernel strengths
   int nxKernel;
   int nyKernel;
   int nfKernel;

#ifdef PV_USE_MPI
   pvdata_t * mpiReductionBuffer;
#endif // PV_USE_MPI

   void set_kernelPatches(PVPatch *** p) {kernelPatches = p;}

   virtual int deleteWeights();
   int initialize_base();
   int initialize(const char * name, HyPerCol * hc,
                  HyPerLayer * pre, HyPerLayer * post,
                  ChannelType channel, const char * filename,
                  InitWeights *weightInit=NULL);
   virtual int createArbors();
   virtual int initPlasticityPatches();
   virtual pvdata_t * allocWeights(PVPatch *** patches, int nPatches, int nxPatch,
         int nyPatch, int nfPatch, int axonId);
   virtual int initializeUpdateTime(PVParams * params);
   virtual PVPatch *** initializeWeights(PVPatch *** arbors, int numPatches,
         const char * filename);

   virtual int calc_dW(int axonId);
   virtual int clear_dW(int axonId);
   virtual int update_dW(int axonId);
   virtual int defaultUpdate_dW(int axonId);
   virtual pvdata_t updateRule_dW(pvdata_t pre, pvdata_t post);

   virtual int updateState(float time, float dt);
   virtual int updateWeights(int axonId);
   virtual float computeNewWeightUpdateTime(float time, float currentUpdateTime);
#ifdef PV_USE_MPI
   virtual int reduceKernels(int axonID);
#endif // PV_USE_MPI
   virtual PVPatch ** readWeights(PVPatch ** patches, int numPatches,
                                     const char * filename);
   virtual int setWPatches(PVPatch ** patches, int arborId);
   virtual int setdWPatches(PVPatch ** patches, int arborId);
};

}

#endif /* KERNELCONN_HPP_ */
