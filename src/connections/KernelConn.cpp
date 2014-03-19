/*
 * KernelConn.cpp
 *
 *  Created on: Aug 6, 2009
 *      Author: gkenyon
 */

#include "KernelConn.hpp"
#include "../normalizers/NormalizeBase.hpp"
#include <assert.h>
#include <float.h>
#include "../io/io.h"
#ifdef USE_SHMGET
   #include <sys/shm.h>
#endif

namespace PV {

KernelConn::KernelConn()
{
   initialize_base();
}

KernelConn::KernelConn(const char * name, HyPerCol * hc) : HyPerConn() {
   KernelConn::initialize_base();
   KernelConn::initialize(name, hc);
}

KernelConn::~KernelConn() {
   deleteWeights();
   free(numKernelActivations);
   #ifdef PV_USE_MPI
      free(mpiReductionBuffer);
   #endif // PV_USE_MPI
}

int KernelConn::initialize_base()
{
   fileType = PVP_KERNEL_FILE_TYPE;
   lastUpdateTime = 0.f;
   plasticityFlag = false;
   normalizeArborsIndividually = false;
   symmetrizeWeightsFlag = false;
#ifdef PV_USE_MPI
   keepKernelsSynchronized_flag = true;
   mpiReductionBuffer = NULL;
#endif // PV_USE_MPI
   return PV_SUCCESS;
   // KernelConn constructor calls HyPerConn::HyPerConn(), which
   // calls HyPerConn::initialize_base().
}

int KernelConn::communicateInitInfo(){
   int status = HyPerConn::communicateInitInfo();
   //Check axonal arbors against windows
   if (pre->getNumWindows() != 1 && pre->getNumWindows() != this->numberOfAxonalArborLists()){
      fprintf(stderr, "KernelConn::Number of windows in %s is %d (calculated from symmetry), while number of arbors in %s is %d. Either some windows or arbors will not be used\n", pre->getName(), pre->getNumWindows(), name, this->numberOfAxonalArborLists());
   }
   return status;
}

int KernelConn::initialize(const char * name, HyPerCol * hc) {
   HyPerConn::initialize(name, hc);
#ifdef PV_USE_OPENCL
//   //don't support GPU accelleration in kernelconn yet
//   ignoreGPUflag=false;
//   //tell the recieving layer to copy gsyn to the gpu, because kernelconn won't be calculating it
//   post->copyChannelToDevice();
#endif

   return PV_SUCCESS;
}

int KernelConn::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = HyPerConn::ioParamsFillGroup(ioFlag);
   ioParam_dWMax(ioFlag);
   ioParam_shmget_flag(ioFlag);
   ioParam_keepKernelsSynchronized(ioFlag);
   ioParam_useWindowPost(ioFlag);
   return status;
}

void KernelConn::ioParam_shmget_flag(enum ParamsIOFlag ioFlag) {
#ifdef USE_SHMGET
   assert(!parent->parameters()->presentAndNotBeenRead(name, "plasticityFlag"));
   parent->ioParamValue(ioFlag, name, "shmget_flag", &shmget_flag, shmget_flag, true/*warnIfAbsent*/);
   if (plasticityFlag && shmget_flag) {
       shmget_flag = false;
       if (parent->columnId()==0) {
          std::cout << "in KernelConn::initialize: " << this->name
                    << ", shmget_flag parameter specified as true, reset to false because plasticity_flag is true"
                    << std::endl;

       }
   }
#else
   if (ioFlag == PARAMS_IO_READ) {
      // mark as read so that shmget_flag doesn't get an unread-parameter warning.
      // This way the same params file can be used with USE_SHMGET on or off.
      parent->parameters()->value(name, "shmget_flag", false, false);
   }
#endif // USE_SHMGET
}

void KernelConn::ioParam_keepKernelsSynchronized(enum ParamsIOFlag ioFlag) {
   assert(!parent->parameters()->presentAndNotBeenRead(name, "plasticityFlag"));
   if (plasticityFlag) {
      parent->ioParamValue(ioFlag, name, "keepKernelsSynchronized", &keepKernelsSynchronized_flag, keepKernelsSynchronized_flag, true/*warnIfAbsent*/);
   }
}

void KernelConn::ioParam_useWindowPost(enum ParamsIOFlag ioFlag) {
   assert(!parent->parameters()->presentAndNotBeenRead(name, "numAxonalArbors"));
   assert(!parent->parameters()->presentAndNotBeenRead(name, "plasticityFlag"));
   if (plasticityFlag && numAxonalArborLists>1) {
      initialWeightUpdateTime = 1.0;
      parent->ioParamValue(ioFlag, name, "useWindowPost", &useWindowPost, useWindowPost);
   }
}

int KernelConn::createArbors() {
#ifdef USE_SHMGET
	if (shmget_flag){
		shmget_id = (int *) calloc(this->numberOfAxonalArborLists(),
				sizeof(int));
		assert(shmget_id != NULL);
		shmget_owner = (bool *) calloc(this->numberOfAxonalArborLists(),
				sizeof(bool));
		assert(shmget_owner != NULL);
	}
#endif
   HyPerConn::createArbors();
   return PV_SUCCESS; //should we check if allocation was successful?
}

int KernelConn::initPlasticityPatches() {
   assert(!parent->parameters()->presentAndNotBeenRead(name, "plasticityFlag"));
#ifdef PV_USE_MPI
   if( getPlasticityFlag() ) {
      assert(!parent->parameters()->presentAndNotBeenRead(name, "keepKernelsSynchronized"));
   }
#endif // PV_USE_MPI
   HyPerConn::initPlasticityPatches();
   return PV_SUCCESS;
}

// use shmget() to save memory on shared memory architectures
//pvdata_t * KernelConn::allocWeights(PVPatch *** patches, int nPatches,
//		int nxPatch, int nyPatch, int nfPatch, int arbor_ID) {
pvdata_t * KernelConn::allocWeights(int nPatches, int nxPatch, int nyPatch, int nfPatch){

	int sx = nfPatch;
	int sy = sx * nxPatch;
	int sp = sy * nyPatch;

	size_t patchSize = sp * sizeof(pvdata_t);
	size_t dataSize = nPatches * patchSize;

	//if (arbor_ID > 0) {  // wDataStart already allocated
//#ifdef USE_SHMGET

	//	if (shmget_flag) {
	//		shmget_owner[arbor_ID] = shmget_owner[0];
	//        shmget_id[arbor_ID] = shmget_id[0];
	//	}
//#endif // SHMGET_DEBUG
	//	assert(this->get_wDataStart(0) != NULL);
	//	return (this->get_wDataStart(0) + sp * nPatches * arbor_ID);
	//}

	// arbor_ID == 0
	size_t arborSize = dataSize * this->numberOfAxonalArborLists();
	pvdata_t * dataPatches = NULL; // (pvdata_t *) calloc(dataSize, sizeof(char));
#ifdef USE_SHMGET
	int arbor_ID = 0;
	if (!shmget_flag) {
		dataPatches = (pvdata_t *) calloc(arborSize, sizeof(char));
	} else {
		shmget_owner[arbor_ID] = true;
		// shmget diagnostics
#define SHMGET_DEBUG
#ifdef SHMGET_DEBUG
		if (arbor_ID == 0 || arbor_ID == (this->numberOfAxonalArborLists()-1)) {
			std::cout << "rank = " << parent->icCommunicator()->commRank();
			std::cout << ", arbor_ID = " << arbor_ID;
		}
#endif // SHMGET_DEBUG
		// dataSize must be a multiple of PAGE_SIZE
		size_t shmget_dataSize = (floor(arborSize / PAGE_SIZE) + 1) * PAGE_SIZE;
		key_t key = IPC_PRIVATE;
		const int max_arbors = 8712;
		key = 11 + (this->getConnectionId() + 1) * max_arbors + arbor_ID; //hopefully unique key identifier for all shared memory associated with this connection arbor
		int shmflg = (IPC_CREAT | IPC_EXCL | 0666);
		char *segptr;

		// check for existing segment associated with this key, delete existing segment if present, then insert barrier to ensure
		// all processes have completed this check before attempting to create new shared memory segment
		int shmget_existing_ID = shmget(key, shmget_dataSize, 0666);
		if (shmget_existing_ID != -1){
			shmid_ds * shmget_ds = NULL;
			int shmctl_status = shmctl(shmget_existing_ID, IPC_RMID,
					shmget_ds);
			std::cout << "shmctl_status = " << shmctl_status << std::endl;
			//			assert(shmget_status==0);
		}
#ifdef PV_USE_MPI
         MPI_Barrier(getParent()->icCommunicator()->communicator());
#endif // PV_USE_MPI


 		/* Open the shared memory segment - create if necessary */
		if ((shmget_id[arbor_ID] = shmget(key, shmget_dataSize, shmflg))
				== -1) {
			if (errno != EEXIST) {
				std::cout << std::endl;
				std::cout << "key = " << key << ", shmget_dataSize = "
						<< shmget_dataSize << ", shmflg = "
						<< shmflg << std::endl;
				perror("shmget: unable to create shared memory segment");
				exit(1);
			}
			/* Segment already exists - try as a client */
			shmget_owner[arbor_ID] = false;
			int shmget_flag2 = (IPC_CREAT | 0666);
			if ((shmget_id[arbor_ID] = shmget(key, shmget_dataSize,
					shmget_flag2)) == -1) {
				perror(
						"shmget: unable to obtain id of existing shared memory segment");
				exit(1);
			}
		}
#ifdef SHMGET_DEBUG
		if (arbor_ID == 0 || arbor_ID == (this->numberOfAxonalArborLists()-1)) {
			std::cout << ", shmget_owner = " << shmget_owner[arbor_ID]
					<< std::endl;
		}
#endif // SHMGET_DEBUG
		/* Attach (map) the shared memory segment into the current process */
		if ((segptr = (char *) shmat(shmget_id[arbor_ID], 0, 0))
				== (char *) -1) {
			perror("shmat: unable to map shared memory segment");
			exit(1);
		}
		dataPatches = (pvdata_t *) segptr;
	}
#else
	dataPatches = (pvdata_t *) calloc(arborSize, sizeof(char));
#endif // USE_SHMGET
	assert(dataPatches != NULL);

	return dataPatches;
}



int KernelConn::deleteWeights()
{
   free(patch2datalookuptable);
   return 0; // HyPerConn destructor will call HyPerConn::deleteWeights()

}

PVPatch ***  KernelConn::initializeWeights(PVPatch *** patches, pvdata_t ** dataStart, int numPatches)
{
   //int arbor = 0;
   int numKernelPatches = getNumDataPatches();
   HyPerConn::initializeWeights(NULL, dataStart, numKernelPatches);
   return patches;
}

int KernelConn::initNumDataPatches()
{
   int nxKernel = (pre->getXScale() < post->getXScale()) ? (int)pow(2,
         post->getXScale() - pre->getXScale()) : 1;
   int nyKernel = (pre->getYScale() < post->getYScale()) ? (int)pow(2,
         post->getYScale() - pre->getYScale()) : 1;
   numDataPatches = pre->getLayerLoc()->nf * nxKernel * nyKernel;
   return PV_SUCCESS;
}

int KernelConn::allocateDataStructures() {
   HyPerConn::allocateDataStructures();
   PVParams * params = parent->parameters();
   assert(!params->presentAndNotBeenRead(name, "plasticityFlag"));

   //Moved to HyPerConn
   //assert(!params->presentAndNotBeenRead(name, "plasticityFlag"));
   //if (plasticityFlag) {
   //   assert(!params->presentAndNotBeenRead(name, "initialWeightUpdateTime"));
   //   assert(!params->presentAndNotBeenRead(name, "weightUpdatePeriod"));
   //   if (parent->getCheckpointReadFlag()==false && weightUpdateTime < parent->simulationTime()) {
   //      while(weightUpdateTime <= parent->simulationTime()) {weightUpdateTime += weightUpdatePeriod;}
   //      if (parent->columnId()==0) {
   //         fprintf(stderr, "Warning: initialWeightUpdateTime of %s \"%s\" less than simulation start time.  Adjusting weightUpdateTime to %f\n",
   //               parent->parameters()->groupKeywordFromName(name), name, weightUpdateTime);
   //      }
   //   }
   //   lastUpdateTime = weightUpdateTime - parent->getDeltaTime();
   //}
#ifdef PV_USE_MPI
   // preallocate buffer for MPI_Allreduce call in reduceKernels
   // Should only call reduceKernels if plasticityFlag is set, so only allocate if it is set.
   assert(!parent->parameters()->presentAndNotBeenRead(name, "plasticityFlag"));
   if (plasticityFlag) {
      const int numPatches = getNumDataPatches();
      const size_t patchSize = nxp*nyp*nfp*sizeof(pvdata_t);
      const size_t localSize = numPatches * patchSize;
      mpiReductionBuffer = (pvdata_t *) malloc(localSize*sizeof(pvdata_t));
      if(mpiReductionBuffer == NULL) {
         if (parent->columnId()==0) {
            fprintf(stderr, "KernelConn::initialize:Unable to allocate memory\n");
         }
         MPI_Barrier(parent->icCommunicator()->communicator());
         exit(PV_FAILURE);
      }
   }
#endif // PV_USE_MPI

   numKernelActivations = (int*) malloc(getNumDataPatches() * sizeof(int));
   for(int ki = 0; ki < getNumDataPatches(); ki++){
      numKernelActivations[ki] = 0;
   }
   return PV_SUCCESS;
}

float KernelConn::minWeight(int arborId)
{
   const int numKernels = getNumDataPatches();
   const int numWeights = nxp * nyp * nfp;
   float min_weight = FLT_MAX;
   for (int iKernel = 0; iKernel < numKernels; iKernel++) {
      pvdata_t * kernelWeights = this->get_wDataHead(arborId, iKernel);
      for (int iWeight = 0; iWeight < numWeights; iWeight++) {
         min_weight = (min_weight < kernelWeights[iWeight]) ? min_weight
               : kernelWeights[iWeight];
      }
   }
   return min_weight;
}

float KernelConn::maxWeight(int arborId)
{
   const int numKernels = getNumDataPatches();
   const int numWeights = nxp * nyp * nfp;
   float max_weight = -FLT_MAX;
   for (int iKernel = 0; iKernel < numKernels; iKernel++) {
      pvdata_t * kernelWeights = this->get_wDataHead(arborId, iKernel);
      for (int iWeight = 0; iWeight < numWeights; iWeight++) {
         max_weight = (max_weight > kernelWeights[iWeight]) ? max_weight
               : kernelWeights[iWeight];
      }
   }
   return max_weight;
}

int KernelConn::calc_dW(int arbor_ID){
   clear_dW(arbor_ID);
   //update_dW(arbor_ID);
   //return PV_BREAK;
   return update_dW(arbor_ID);
}

int KernelConn::clear_dW(int arbor_ID) {
   // zero dwDataStart
   for(int kArbor = 0; kArbor < numberOfAxonalArborLists(); kArbor++){
      for(int kKernel = 0; kKernel < getNumDataPatches(); kKernel++){
         int syPatch = syp;
         int nkPatch = nfp * nxp;
         float * dWeights = get_dwDataHead(arbor_ID,kKernel);
         for(int kyPatch = 0; kyPatch < nyp; kyPatch++){
            for(int kPatch = 0; kPatch < nkPatch; kPatch++){
               dWeights[kPatch] = 0.0f;
            }
            dWeights += syPatch;
         }
      }
   }
   return PV_BREAK;
   //return PV_SUCCESS;
}

int KernelConn::update_dW(int arbor_ID) {
   // Typically override this method with a call to defaultUpdate_dW(arbor_ID)
//	for(int arborID=0;arborID<numberOfAxonalArborLists();arborID++) {
       int status = defaultUpdate_dW(arbor_ID);  // calculate new weights from changes
//       if (status == PV_BREAK) {break;}
//       assert(status == PV_SUCCESS);
//    }
   return status;
   //return PV_SUCCESS;
}

int KernelConn::defaultUpdateInd_dW(int arbor_ID, int kExt){
   const pvdata_t * preactbuf = preSynapticLayer()->getLayerData(getDelay(arbor_ID));
   const pvdata_t * postactbuf = postSynapticLayer()->getLayerData(); 
   const PVLayerLoc * preLoc = pre->getLayerLoc();
   const PVLayerLoc * postLoc = post->getLayerLoc();
   int sya = (post->getLayerLoc()->nf * (post->getLayerLoc()->nx + 2*post->getLayerLoc()->nb));

   pvdata_t preact = preactbuf[kExt];
   if (skipPre(preact)) return PV_CONTINUE;
   //update numKernelActivations

   int kernelIndex = patchIndexToDataIndex(kExt);
   //Only increment if kernelIndex is restricted
   int nxExt = preLoc->nx + 2*preLoc->nb;
   int nyExt = preLoc->ny + 2*preLoc->nb;
   int nf = preLoc->nf;
   int extX = kxPos(kExt, nxExt, nyExt, nf);
   int extY = kyPos(kExt, nxExt, nyExt, nf);
   if(extX >= preLoc->nb && extX < preLoc->nx + preLoc->nb &&
      extY >= preLoc->nb && extY < preLoc->ny + preLoc->nb){
      numKernelActivations[kernelIndex]++;
   }

   //if (preact == 0.0f) continue;
   bool inWindow = true;
   // only check inWindow if number of arbors > 1
   if (this->numberOfAxonalArborLists()>1){
      if(useWindowPost){
         int kPost = layerIndexExt(kExt, preLoc, postLoc);
         inWindow = post->inWindowExt(arbor_ID, kPost);
      }
      else{
         inWindow = pre->inWindowExt(arbor_ID, kExt);
      }
      if(!inWindow) return PV_CONTINUE;
   }
   PVPatch * weights = getWeights(kExt,arbor_ID);
   size_t offset = getAPostOffset(kExt, arbor_ID);
   int ny = weights->ny;
   int nk = weights->nx * nfp;
   const pvdata_t * postactRef = &postactbuf[offset];
   pvdata_t * dwdata = get_dwData(arbor_ID, kExt);
   int lineoffsetw = 0;
   int lineoffseta = 0;
   for( int y=0; y<ny; y++ ) {
      for( int k=0; k<nk; k++ ) {
         pvdata_t aPost = postactRef[lineoffseta+k];
         dwdata[lineoffsetw + k] += updateRule_dW(preact, aPost);
      }
      lineoffsetw += syp;
      lineoffseta += sya;
   }


   return PV_SUCCESS;
}

int KernelConn::normalize_dW(int arbor_ID){
   int numKernelIndices = getNumDataPatches();

   //Do mpi to update numKernelActivationss 
#ifdef PV_USE_MPI
   int ierr = MPI_Allreduce(MPI_IN_PLACE, numKernelActivations , numKernelIndices, MPI_INT, MPI_SUM, parent->icCommunicator()->communicator());
#endif

   // Divide by numKernelActivations in this timestep
   for( int kernelindex=0; kernelindex<numKernelIndices; kernelindex++ ) {
      //Calculate pre feature index from patch index
      //TODO right now it's dividing the divisor by nprocs. This is a hack. Proper fix is to update all connections overwriting
      //update_dW to do dwNormalization in this way and take out the divide by nproc in reduceKernels.
      const int nProcs = parent->icCommunicator()->numCommColumns() * parent->icCommunicator()->numCommRows();
      double divisor = numKernelActivations[kernelindex]/nProcs;
      if(divisor != 0){
         int numpatchitems = nxp*nyp*nfp;
         pvdata_t * dwpatchdata = get_dwDataHead(arbor_ID,kernelindex);
         for( int n=0; n<numpatchitems; n++ ) {
            dwpatchdata[n] /= divisor;
         }
      }
   }
   return PV_SUCCESS;
}

int KernelConn::defaultUpdate_dW(int arbor_ID) {
   // compute dW but don't add them to the weights yet.
   // That takes place in reduceKernels, so that the output is
   // independent of the number of processors.
   int nExt = preSynapticLayer()->getNumExtended();
   
   int numKernelIndices = getNumDataPatches();

   //Reset numKernelActivations
   for(int ki = 0; ki < numKernelIndices; ki++){
      numKernelActivations[ki] = 0;
   }
   
   for(int kExt=0; kExt<nExt;kExt++) {
      defaultUpdateInd_dW(arbor_ID, kExt);
   }

   normalize_dW(arbor_ID);

//   //Do mpi to update numKernelActivationss 
//#ifdef PV_USE_MPI
//   int ierr = MPI_Allreduce(MPI_IN_PLACE, numKernelActivations , numKernelIndices, MPI_INT, MPI_SUM, parent->icCommunicator()->communicator());
//#endif

   //// Divide by numKernelActivations in this timestep
   //for( int kernelindex=0; kernelindex<numKernelIndices; kernelindex++ ) {
   //   //Calculate pre feature index from patch index
   //   //TODO right now it's dividing the divisor by nprocs. This is a hack. Proper fix is to update all connections overwriting
   //   //update_dW to do dwNormalization in this way and take out the divide by nproc in reduceKernels.
   //   const int nProcs = parent->icCommunicator()->numCommColumns() * parent->icCommunicator()->numCommRows();
   //   double divisor = numKernelActivations[kernelindex]/nProcs;
   //   if(divisor != 0){
   //      int numpatchitems = nxp*nyp*nfp;
   //      pvdata_t * dwpatchdata = get_dwDataHead(arbor_ID,kernelindex);
   //      for( int n=0; n<numpatchitems; n++ ) {
   //         dwpatchdata[n] /= divisor;
   //      }
   //   }
   //}

   // Divide by (numNeurons/numKernels)
   //int divisor = pre->getNumNeurons()/numKernelIndices;
   //assert( divisor*numKernelIndices == pre->getNumNeurons() );
   //for( int kernelindex=0; kernelindex<numKernelIndices; kernelindex++ ) {
   //   int numpatchitems = nxp*nyp*nfp;
   //   pvdata_t * dwpatchdata = get_dwDataHead(arbor_ID,kernelindex);
   //   for( int n=0; n<numpatchitems; n++ ) {
   //      dwpatchdata[n] /= divisor;
   //   }
   //}

   //Moved to HyPerConn
   //lastUpdateTime = parent->simulationTime();

   return PV_SUCCESS;
}

pvdata_t KernelConn::updateRule_dW(pvdata_t pre, pvdata_t post) {
   return dWMax * pre * post;
}

int KernelConn::updateState(double timef, double dt) {
   int status = PV_SUCCESS;
   if( !plasticityFlag ) {
      return status;
   }
   update_timer->start();

   //Now done in HyPerConn::updateStateWrapper
   //if( timef >= weightUpdateTime) {
   //   computeNewWeightUpdateTime(timef, weightUpdateTime);

      for(int arborID=0;arborID<numberOfAxonalArborLists();arborID++) {
         status = calc_dW(arborID);  // calculate changes in weights
         if (status == PV_BREAK) {break;}
         assert(status == PV_SUCCESS);
      }

#ifdef PV_USE_MPI
      if (keepKernelsSynchronized_flag
            || parent->simulationTime() >= parent->getStopTime()-parent->getDeltaTime()) {
         for (int arborID = 0; arborID < numberOfAxonalArborLists(); arborID++) {
            status = reduceKernels(arborID); // combine partial changes in each column
            if (status == PV_BREAK) {
               break;
            }
            assert(status == PV_SUCCESS);
         }
      }
#endif // PV_USE_MPI

      for(int arborID=0;arborID<numberOfAxonalArborLists();arborID++) {
         status = updateWeights(arborID);  // calculate new weights from changes
         if (status == PV_BREAK) {break;}
         assert(status == PV_SUCCESS);
      }
      normalizeWeights();
   //} // time > weightUpdateTime

   update_timer->stop();
   return PV_SUCCESS;
} // updateState

int KernelConn::updateWeights(int arbor_ID){
   lastUpdateTime = parent->simulationTime();
   // add dw to w
   // never use shmget if plasticity flag == true
//#ifdef USE_SHMGET
//   if (shmget_flag && !shmget_owner[arbor_ID]){
//         return PV_BREAK;
//      }
//#endif
   for(int kArbor = 0; kArbor < this->numberOfAxonalArborLists(); kArbor++){
//#ifdef USE_SHMGET
//      volatile pvdata_t * w_data_start = get_wDataStart(kArbor);
//#else
      pvdata_t * w_data_start = get_wDataStart(kArbor);
//#endif
//
      for( int k=0; k<nxp*nyp*nfp*getNumDataPatches(); k++ ) {
         w_data_start[k] += get_dwDataStart(kArbor)[k];
      }
   }
   return PV_BREAK;
}

//Moved to HyPerConn
//double KernelConn::computeNewWeightUpdateTime(double time, double currentUpdateTime) {
//   // Is only called by KernelConn::updateState if plasticityFlag is true
//   weightUpdateTime += weightUpdatePeriod;
//   return weightUpdateTime;
//}

#ifdef PV_USE_MPI
int KernelConn::reduceKernels(const int arborID) {
   assert(plasticityFlag && mpiReductionBuffer);
   Communicator * comm = parent->icCommunicator();
   const MPI_Comm mpi_comm = comm->communicator();
   int ierr;
   const int nxProcs = comm->numCommColumns();
   const int nyProcs = comm->numCommRows();
   const int nProcs = nxProcs * nyProcs;
   if (nProcs == 1){
      return PV_BREAK;
   }
   const int numPatches = getNumDataPatches();
   const size_t patchSize = nxp*nyp*nfp; // *sizeof(pvdata_t);
   const size_t localSize = numPatches * patchSize;
   const size_t arborSize = localSize * this->numberOfAxonalArborLists();

   ierr = MPI_Allreduce(MPI_IN_PLACE, this->get_dwDataStart(0), arborSize, MPI_FLOAT, MPI_SUM, mpi_comm);
   pvdata_t * dW_data = this->get_dwDataStart(0);
   for (int i_dW = 0; i_dW < arborSize; i_dW++){
	   dW_data[i_dW] /= nProcs;
   }

//#ifdef PV_USE_MPI
//   //Reduce all of numKernelActivationss from all processors
//   ierr = MPI_Allreduce(MPI_IN_PLACE, numKernelActivations , numPatches, MPI_INT, MPI_SUM, mpi_comm);
//   //Reduce all of dw in each processor
//   ierr = MPI_Allreduce(MPI_IN_PLACE, this->get_dwDataStart(0), arborSize, MPI_FLOAT, MPI_SUM, mpi_comm);
//#endif
//
//   pvdata_t * dW_data = this->get_dwDataStart(0);
//   //Arbor size is the localSize of each arbor * numArbors
//   //So this is looping through patch size, numPatches, and arbors
//   for (int i_dW = 0; i_dW < arborSize; i_dW++){
//      int kernelIndex = ((int)i_dW/(int)patchSize) % numPatches;
//      assert(kernelIndex >= 0 && kernelIndex < numPatches);
//      //If defaultUpdate_dW is not called, everything in numActive should be 0
//      //which will prevent this dw normalization
//      if(numKernelActivations[kernelIndex] != 0){
//         dW_data[i_dW] /= numKernelActivations[kernelIndex];
//      }
//   }
//
//
//   //Reset numKernelActivations
//   for(int ki = 0; ki < numPatches; ki++){
//      numKernelActivations[ki] = 0;
//   }

//
//   // Divide by numKernelActivations in this timestep
//   for( int kernelindex=0; kernelindex<numKernelIndices; kernelindex++ ) {
//      //Calculate pre feature index from patch index
//      int divisor = numKernelActivations[kernelindex];
//      if(divisor != 0){
//         int numpatchitems = nxp*nyp*nfp;
//         pvdata_t * dwpatchdata = get_dwDataHead(arbor_ID,kernelindex);
//         for( int n=0; n<numpatchitems; n++ ) {
//            dwpatchdata[n] /= divisor;
//         }
//      }
//   }

   // Copy this column's weights into mpiReductionBuffer
   //TODO!!! should do mem copy here since weights stored in contiguous memory
   //memcpy(mpiReductionBuffer, this->get_dwDataStart(arborID), localSize);

/*
   int idx = 0;
   for (int k = 0; k < numPatches; k++) {
      const pvdata_t * data = get_dwDataHead(arborID,k);

      for (int y = 0; y < nyp; y++) {
         for (int x = 0; x < nxp; x++) {
            for (int f = 0; f < nfp; f++) {
               mpiReductionBuffer[idx] = data[x*sxp + y*syp + f*sfp];
               idx++;
            }
         }
      }
   }
*/

   // MPI_Allreduce combines all processors' buffers and puts the common result
   // into each processor's buffer.
   //ierr = MPI_Allreduce(MPI_IN_PLACE, mpiReductionBuffer, localSize, MPI_FLOAT, MPI_SUM, mpi_comm);
   // TODO error handling

   // mpiReductionBuffer now holds the sum over all processes.
   // Divide by number of processes to get average and copy back to patches
   //TODO!!! should do mem copy here since weights stored in contiguous memory
   //memcpy(this->get_dwDataStart(), mpiReductionBuffer, localSize);

/*
   idx = 0;
   for (int k = 0; k < numPatches; k++) {
      pvdata_t * data = get_dwDataHead(arborID,k); // p->data;

      for (int y = 0; y < nyp; y++) {
         for (int x = 0; x < nxp; x++) {
            for (int f = 0; f < nfp; f++) {
               data[x*sxp + y*syp + f*sfp] = mpiReductionBuffer[idx]/nProcs;
               idx++;
            }
         }
      }
   }
*/

   //return PV_SUCCESS;
   return PV_BREAK;
}
#endif // PV_USE_MPI

void KernelConn::initPatchToDataLUT() {
   int numWeightPatches=getNumWeightPatches();

   patch2datalookuptable=(int *) calloc(numWeightPatches, sizeof(int));
   for(int i=0; i<numWeightPatches; i++) {
      int kernelindex=patchIndexToDataIndex(i);
      patch2datalookuptable[i]=kernelindex;
   }

}
int KernelConn::patchToDataLUT(int patchIndex) {
   // This uses a look-up table
   return patch2datalookuptable[patchIndex];
}

#ifdef OBSOLETE // Marked obsolete April 11, 2013.  Implementing the new NormalizeBase class hierarchy.
int KernelConn::normalizeWeights(PVPatch ** patches, pvdata_t ** dataStart,
		int numPatches, int arborId) {
	int status = PV_SUCCESS;
	const int num_kernels = getNumDataPatches();

	// symmetrize before normalization
	if (symmetrizeWeightsFlag) {
		for (int kArbor = 0; kArbor < this->numberOfAxonalArborLists();
				kArbor++) {
			status = symmetrizeWeights(dataStart[arborId], num_kernels, kArbor);
			assert( (status == PV_SUCCESS) || (status == PV_BREAK));
		}
	}
	status = HyPerConn::normalizeWeights(NULL, dataStart, num_kernels, arborId);
	assert( (status == PV_SUCCESS) || (status == PV_BREAK));
	return status;
}

int KernelConn::symmetrizeWeights(pvdata_t * dataStart, int numPatches, int arborId)
{
   int status = PV_SUCCESS;
   if (arborId == 0)
      printf("Entering KernelConn::symmetrizeWeights for connection \"%s\"\n", name);
   assert(pre->clayer->loc.nf==post->clayer->loc.nf);

   pvdata_t * symPatches = (pvdata_t *) calloc(nxp*nyp*nfp*numPatches, sizeof(pvdata_t));
   assert(symPatches != NULL);

   const int sy = nxp * nfp;
   const float deltaTheta = PI / nfp;
   const float offsetTheta = 0.5f * deltaTheta;
   const int kyMid = nyp / 2;
   const int kxMid = nxp / 2;
   for (int iSymKernel = 0; iSymKernel < numPatches; iSymKernel++) {
      pvdata_t * symW = symPatches + iSymKernel*nxp*nyp*nfp;
      float symTheta = offsetTheta + iSymKernel * deltaTheta;
      for (int kySym = 0; kySym < nyp; kySym++) {
         float dySym = kySym - kyMid;
         for (int kxSym = 0; kxSym < nxp; kxSym++) {
            float dxSym = kxSym - kxMid;
            float distSym = sqrt(dxSym * dxSym + dySym * dySym);
            if (distSym > abs(kxMid > kyMid ? kxMid : kyMid)) {
               continue;
            }
            float dyPrime = dySym * cos(symTheta) - dxSym * sin(symTheta);
            float dxPrime = dxSym * cos(symTheta) + dySym * sin(symTheta);
            for (int kfSym = 0; kfSym < nfp; kfSym++) {
               int kDf = kfSym - iSymKernel;
               int iSymW = kfSym + nfp * kxSym + sy * kySym;
               for (int iKernel = 0; iKernel < nfp; iKernel++) {
                  // PVPatch * kerWp = getKernelPatch(arborId, iKernel);
                  pvdata_t * kerW = get_wDataStart(arborId) + iKernel*nxp*nyp*nfp;
                  int kfRot = iKernel + kDf;
                  if (kfRot < 0) {
                     kfRot = nfp + kfRot;
                  }
                  else {
                     kfRot = kfRot % nfp;
                  }
                  float rotTheta = offsetTheta + iKernel * deltaTheta;
                  float yRot = dyPrime * cos(rotTheta) + dxPrime * sin(rotTheta);
                  float xRot = dxPrime * cos(rotTheta) - dyPrime * sin(rotTheta);
                  yRot += kyMid;
                  xRot += kxMid;
                  // should find nearest neighbors and do weighted average
                  int kyRot = yRot + 0.5f;
                  int kxRot = xRot + 0.5f;
                  int iRotW = kfRot + nfp * kxRot + sy * kyRot;
                  symW[iSymW] += kerW[iRotW] / nfp;
               } // kfRot
            } // kfSymm
         } // kxSym
      } // kySym
   } // iKernel
   const int num_weights = nfp * nxp * nyp;
   for (int iKernel = 0; iKernel < numPatches; iKernel++) {
      pvdata_t * kerW = get_wDataStart(arborId)+iKernel*nxp*nyp*nfp;
      pvdata_t * symW = symPatches + iKernel*nxp*nyp*nfp;
      for (int iW = 0; iW < num_weights; iW++) {
         kerW[iW] = symW[iW];
      }
   } // iKernel

   free(symPatches);

   printf("Exiting KernelConn::symmetrizeWeights for connection \"%s\"\n", name);
   return status;
}
#endif // OBSOLETE

int KernelConn::writeWeights(double timef, bool last) {
   const int numPatches = getNumDataPatches();
   return HyPerConn::writeWeights(NULL, get_wDataStart(), numPatches, NULL, timef, writeCompressedWeights, last);
}

int KernelConn::writeWeights(const char * filename) {
   return HyPerConn::writeWeights(NULL, get_wDataStart(), getNumDataPatches(), filename, parent->simulationTime(), writeCompressedWeights, /*last*/true);
}

int KernelConn::checkpointRead(const char * cpDir, double * timef) {
   // Can't inherit from HyPerConn::checkpointRead because first argument to weightsInitObject->initializeWeights must be different.
   clearWeights(get_wDataStart(), getNumDataPatches(), nxp, nyp, nfp);

   char path[PV_PATH_MAX];
   int status = checkpointFilename(path, PV_PATH_MAX, cpDir);
   assert(status==PV_SUCCESS);
   InitWeights * weightsInitObject = new InitWeights(this);
   weightsInitObject->readWeights(NULL, get_wDataStart(), getNumDataPatches(), path, timef);
   delete weightsInitObject;
   //Moved to HyPerConn
   //status = parent->readScalarFromFile(cpDir, getName(), "lastUpdateTime", &lastUpdateTime, lastUpdateTime);
   //assert(status == PV_SUCCESS);
   //status = parent->readScalarFromFile(cpDir, getName(), "weightUpdateTime", &weightUpdateTime, weightUpdateTime);
   //assert(status == PV_SUCCESS);
   //if (this->plasticityFlag &&  weightUpdateTime<parent->simulationTime()) {
   //   // simulationTime() may have been changed by HyPerCol::checkpoint, so this repeats the sanity check on weightUpdateTime in allocateDataStructures
   //   while(weightUpdateTime <= parent->simulationTime()) {weightUpdateTime += weightUpdatePeriod;}
   //   if (parent->columnId()==0) {
   //      fprintf(stderr, "Warning: initialWeightUpdateTime of %s \"%s\" less than simulation start time.  Adjusting weightUpdateTime to %f\n",
   //            parent->parameters()->groupKeywordFromName(name), name, weightUpdateTime);
   //   }
   //}
   return PV_SUCCESS;
}

int KernelConn::checkpointWrite(const char * cpDir) {
   char filename[PV_PATH_MAX];
   int status = checkpointFilename(filename, PV_PATH_MAX, cpDir);
   assert(status==PV_SUCCESS);
//#ifdef PV_USE_MPI
   if (!keepKernelsSynchronized_flag) {
      for (int arbor_id = 0; arbor_id < this->numberOfAxonalArborLists(); arbor_id++) {
         reduceKernels(arbor_id);
      }
   }
//#endif // PV_USE_MPI
   //Moved to HyPerConn
   //status = parent->writeScalarToFile(cpDir, getName(), "lastUpdateTime", lastUpdateTime);
   //assert(status==PV_SUCCESS);
   //status = parent->writeScalarToFile(cpDir, getName(), "weightUpdateTime", weightUpdateTime);
   //assert(status==PV_SUCCESS);
   return HyPerConn::writeWeights(NULL, get_wDataStart(), getNumDataPatches(), filename, parent->simulationTime(), writeCompressedCheckpoints, /*last*/true);
}

int KernelConn::patchIndexToDataIndex(int patchIndex, int * kx/*default=NULL*/, int * ky/*default=NULL*/, int * kf/*default=NULL*/) {
   return calcUnitCellIndex(patchIndex, kx, ky, kf);
}

int KernelConn::dataIndexToUnitCellIndex(int dataIndex, int * kx/*default=NULL*/, int * ky/*default=NULL*/, int * kf/*default=NULL*/) {
   int nfUnitCell = pre->getLayerLoc()->nf;
   int nxUnitCell = zUnitCellSize(pre->getXScale(), post->getXScale());
   int nyUnitCell = zUnitCellSize(pre->getYScale(), post->getYScale());
   assert( dataIndex >= 0 && dataIndex < nxUnitCell*nyUnitCell*nfUnitCell );
   if(kx) *kx = kxPos(dataIndex, nxUnitCell, nyUnitCell, nfUnitCell);
   if(ky) *ky = kyPos(dataIndex, nxUnitCell, nyUnitCell, nfUnitCell);
   if(kf) *kf = featureIndex(dataIndex, nxUnitCell, nyUnitCell, nfUnitCell);
   return dataIndex;
}

int KernelConn::getReciprocalWgtCoordinates(int kx, int ky, int kf, int kernelidx, int * kxRecip, int * kyRecip, int * kfRecip, int * kernelidxRecip) {
   int status = PV_SUCCESS;

   assert( kx>=0 && kx<nxp && ky>=0 && ky<nyp && kf>=0 && kf<nfp && kernelidx>=0 && kernelidx<getNumDataPatches());
   if( status == PV_SUCCESS ) {
      int nxUnitCell = zUnitCellSize(pre->getXScale(), post->getXScale());
      int nyUnitCell = zUnitCellSize(pre->getYScale(), post->getYScale());
      int nfUnitCell = pre->getLayerLoc()->nf;

      int nxUnitCellRecip = zUnitCellSize(post->getXScale(), pre->getXScale());
      int nyUnitCellRecip = zUnitCellSize(post->getYScale(), pre->getYScale());
      int nfUnitCellRecip = post->getLayerLoc()->nf;

      double xScaleFactor = pow(2,pre->getXScale()-post->getXScale()); // many-to-one connections have xScaleFactor<1; one-to-many, >1.
      double yScaleFactor = pow(2,pre->getYScale()-post->getYScale());

      int kxKern = kxPos(kernelidx, nxUnitCell, nyUnitCell, nfUnitCell);
      int kyKern = kyPos(kernelidx, nxUnitCell, nyUnitCell, nfUnitCell);
      int kfKern = featureIndex(kernelidx, nxUnitCell, nyUnitCell, nfUnitCell);

      int xInPostCell = kx % nxUnitCellRecip;
      int yInPostCell = ky % nyUnitCellRecip;

      *kxRecip = (int) ((nxp-1-kx)/xScaleFactor) + kxKern;
      *kyRecip = (int) ((nyp-1-ky)/yScaleFactor) + kyKern;
      *kfRecip = kfKern;
      *kernelidxRecip = kIndex(xInPostCell, yInPostCell, kf, nxUnitCellRecip, nyUnitCellRecip, nfUnitCellRecip);
   }

   return status;
}


} // namespace PV

