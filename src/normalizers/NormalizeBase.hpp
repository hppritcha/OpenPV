/*
 * NormalizeBase.hpp
 *
 *  Created on: Apr 5, 2013
 *      Author: pschultz
 */

#ifndef NORMALIZEBASE_HPP_
#define NORMALIZEBASE_HPP_

#include "../connections/HyPerConn.hpp"
#include <assert.h>

namespace PV {

class NormalizeBase {
// Member functions
public:
   // no public constructor; only subclasses can be constructed directly
   virtual ~NormalizeBase() = 0;

   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag);

   /**
    * Appends the indicated connection to the list of connections for this normalizer
    */
   int addConnToList(HyPerConn * newConn);

   /**
    * The public interface for normalizing weights.
    * If normalizeOnInitialize is true and the simulation time is startTime(),
    * or if normalizeOnWeightUpdate is true and the simulation time is the conn's lastUpdateTime,
    * it calls the (virtual protected) method normalizeWeights
    */
   int normalizeWeightsWrapper();

   const char * getName() {return name;}
   const float getStrength() {return strength;}
   const float getNormalizeCutoff() {return normalize_cutoff;}
   const bool  getSymmetrizeWeightsFlag() {return symmetrizeWeightsFlag;}
   const bool  getNormalizeFromPostPerspectiveFlag() {return normalizeFromPostPerspective;}
   const bool  getNormalizeArborsIndividuallyFlag() {return normalizeArborsIndividually;}

protected:
   NormalizeBase();
   int initialize(const char * name, HyPerCol * hc, HyPerConn ** connectionList, int numConns);

   virtual void ioParam_strength(enum ParamsIOFlag ioFlag);
   virtual void ioParam_rMinX(enum ParamsIOFlag ioFlag);
   virtual void ioParam_rMinY(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalize_cutoff(enum ParamsIOFlag ioFlag);
   virtual void ioParam_symmetrizeWeights(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalizeFromPostPerspective(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalizeArborsIndividually(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalizeOnInitialize(enum ParamsIOFlag ioFlag);
   virtual void ioParam_normalizeOnWeightUpdate(enum ParamsIOFlag ioFlag);

   virtual int normalizeWeights();
   int accumulateSum(pvwdata_t * dataPatchStart, int weights_in_patch, double * sum);
   int accumulateSumShrunken(pvwdata_t * dataPatchStart, double * sum,
   		int nxpShrunken, int nypShrunken, int offsetShrunken, int xPatchStride, int yPatchStride);
   int accumulateSumSquared(pvwdata_t * dataPatchStart, int weights_in_patch, double * sumsq);
   int accumulateSumSquaredShrunken(pvwdata_t * dataPatchStart, double * sumsq,
   		int nxpShrunken, int nypShrunken, int offsetShrunken, int xPatchStride, int yPatchStride);
   int accumulateMax(pvwdata_t * dataPatchStart, int weights_in_patch, float * max);
   int applyThreshold(pvwdata_t * dataPatchStart, int weights_in_patch, float wMax); // weights less than normalize_cutoff*max(weights) are zeroed out
   int applyRMin(pvwdata_t * dataPatchStart, float rMinX, float rMinY,
			int nxp, int nyp, int xPatchStride, int yPatchStride);
   int symmetrizeWeights(HyPerConn * conn); // may be used by several subclasses
   static void normalizePatch(pvwdata_t * dataStart, int weights_per_patch, float multiplier);
   HyPerCol * parent() { return parentHyPerCol; }

private:
   int initialize_base();

// Member variables
protected:
   char * name;
   HyPerCol * parentHyPerCol;
   HyPerConn ** connectionList;
   int numConnections;
   float strength;                    // Value to normalize to; precise interpretation depends on normalization method
   float rMinX, rMinY;                // zero all weights within rectangle rMinxY, rMInY aligned with center of patch
   float normalize_cutoff;            // If true, weights with abs(w)<max(abs(w))*normalize_cutoff are truncated to zero.
   bool symmetrizeWeightsFlag;        // Whether to call symmetrizeWeights.  Only meaningful if pre->nf==post->nf and connection is one-to-one
   bool normalizeFromPostPerspective; // If false, group all weights with a common presynaptic neuron for normalizing.  If true, group all weights with a common postsynaptic neuron
                                      // Only meaningful (at least for now) for KernelConns using sum of weights or sum of squares normalization methods.

   bool normalizeArborsIndividually;  // If true, each arbor is treated as its own connection.  If false, each patch groups all arbors together and normalizes them in common.

   bool normalizeOnInitialize;        // Whether to normalize weights when setting the weights' initial values
   bool normalizeOnWeightUpdate;      // Whether to normalize weights when the weights have been updated
}; // end of class NormalizeBase

} // end namespace PV

#endif /* NORMALIZEBASE_HPP_ */
