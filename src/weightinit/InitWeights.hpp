/*
 * InitWeights.hpp
 *
 *  Created on: Aug 5, 2011
 *      Author: kpeterson
 */

#ifndef INITWEIGHTS_HPP_
#define INITWEIGHTS_HPP_

#include "../include/pv_common.h"
#include "../include/pv_types.h"
#include "../io/PVParams.hpp"
#include "../connections/HyPerConn.hpp"
#include "InitWeightsParams.hpp"
//#include "InitGauss2DWeightsParams.hpp"


namespace PV {

//class HyPerCol;
//lass HyPerLayer;
class InitWeightsParams;
//class InitGauss2DWeightsParams;

class InitWeights {
public:
   InitWeights();
   virtual ~InitWeights();

   /*
    * Although initializeWeights is virtual, in general it should be possible for subclasses
    * to inherit InitWeights::initializeWeights, and only override calcWeights.
    * The method is nevertheless virtual to allow special cases (e.g. BIDS)
    */
   virtual int initializeWeights(PVPatch *** patches, pvdata_t ** dataStart,
			const char * filename, HyPerConn * callingConn,
			double * timef = NULL);
   virtual InitWeightsParams * createNewWeightParams(HyPerConn * callingConn);

   virtual int calcWeights(/* PVPatch * patch */pvdata_t * dataStart,
			int patchIndex, int arborId, InitWeightsParams *weightParams);

   virtual int readWeights(PVPatch *** patches, pvdata_t ** dataStart, int numPatches,
                           const char * filename, HyPerConn * callingConn, double * time=NULL);

   virtual int zeroWeightsOutsideShrunkenPatch(PVPatch *** patches, HyPerConn * callingConn);

protected:
   virtual int initRNGs(HyPerConn * conn, bool isKernel) { return PV_SUCCESS; }

private:
   int initialize_base();

};

} /* namespace PV */
#endif /* INITWEIGHTS_HPP_ */
