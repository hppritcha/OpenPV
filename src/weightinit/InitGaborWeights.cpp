/*
 * InitGaborWeights.cpp
 *
 *  Created on: Aug 13, 2011
 *      Author: kpeterson
 */

#include "InitGaborWeights.hpp"
#include "InitGaborWeightsParams.hpp"

namespace PV {

InitGaborWeights::InitGaborWeights(HyPerConn * conn)
{
   initialize_base();
   initialize(conn);
}

InitGaborWeights::InitGaborWeights()
{
   initialize_base();
}

InitGaborWeights::~InitGaborWeights()
{
}

int InitGaborWeights::initialize_base() {
   return PV_SUCCESS;
}

int InitGaborWeights::initialize(HyPerConn * conn) {
   int status = InitGauss2DWeights::initialize(conn);
   return status;
}

InitWeightsParams * InitGaborWeights::createNewWeightParams() {
   InitWeightsParams * tempPtr = new InitGaborWeightsParams(callingConn);
   return tempPtr;
}

int InitGaborWeights::calcWeights(/* PVPatch * patch */ pvdata_t * dataStart, int patchIndex, int arborId) {

   InitGaborWeightsParams *weightParamPtr = dynamic_cast<InitGaborWeightsParams*>(weightParams);


   if(weightParamPtr==NULL) {
      fprintf(stderr, "Failed to recast pointer to weightsParam!  Exiting...");
      exit(1);
   }


   weightParamPtr->calcOtherParams(patchIndex);

   gaborWeights(dataStart, weightParamPtr);

   return PV_SUCCESS; // return 1;

}

int InitGaborWeights::gaborWeights(/* PVPatch * patch */ pvdata_t * dataStart, InitGaborWeightsParams * weightParamPtr) {
   //load necessary params:
   int nfPatch_tmp = weightParamPtr->getnfPatch();
   int nyPatch_tmp = weightParamPtr->getnyPatch();
   int nxPatch_tmp = weightParamPtr->getnxPatch();
   float aspect=weightParamPtr->getAspect();
   float shift=weightParamPtr->getShift();
   float lambda=weightParamPtr->getlambda();
   float phi=weightParamPtr->getphi();
   bool invert=weightParamPtr->getinvert();
   //int numFlanks=weightParamPtr->getnumFlanks();
   float sigma=weightParamPtr->getSigma();
   int sx_tmp=weightParamPtr->getsx();
   int sy_tmp=weightParamPtr->getsy();
   int sf_tmp=weightParamPtr->getsf();
   double r2Max=weightParamPtr->getr2Max();

   // pvdata_t * w_tmp = patch->data;


   for (int fPost = 0; fPost < nfPatch_tmp; fPost++) {
      float thPost = weightParamPtr->calcThPost(fPost);
      for (int jPost = 0; jPost < nyPatch_tmp; jPost++) {
         float yDelta = weightParamPtr->calcYDelta(jPost);
        for (int iPost = 0; iPost < nxPatch_tmp; iPost++) {
            float xDelta = weightParamPtr->calcXDelta(iPost);

            // rotate the reference frame by th ((x,y) is center of patch (0,0))
            float xp = +xDelta * cosf(thPost) + yDelta * sinf(thPost);
            float yp = -xDelta * sinf(thPost) + yDelta * cosf(thPost);

            float factor = cos(2.0f*PI*yp/lambda + phi);
            if (fabs(yp/lambda) > 3.0f/4.0f) factor = 0.0f;  // phase < 3*PI/2 (no second positive band)

            float d2 = xp * xp + (aspect * (yp - shift) * aspect * (yp - shift));
            float wt = factor * expf(-d2 / (2.0f*sigma*sigma));
            int index = iPost * sx_tmp + jPost * sy_tmp + fPost * sf_tmp;

            if (xDelta*xDelta + yDelta*yDelta > r2Max) {
               dataStart[index] = 0.0f;
            }
            else {
               if (invert) wt *= -1.0f;
               if (wt < 0.0f) wt = 0.0f;       // clip negative values
               dataStart[index] = wt;
            }


         }
      }
   }


   return 0;
}

} /* namespace PV */
