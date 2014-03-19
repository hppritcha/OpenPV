/*
 * PointLIFProbe.cpp
 *
 *  Created on: Mar 10, 2009
 *      Author: rasmussn
 */

#include "PointLIFProbe.hpp"
#include "../layers/HyPerLayer.hpp"
#include "../layers/LIF.hpp"
#include <string.h>
#include <assert.h>

namespace PV {

PointLIFProbe::PointLIFProbe() : PointProbe()
{
   initPointLIFProbe_base();
   // Derived classes of PointLIFProbe should use this PointLIFProbe constructor, and call initPointLIFProbe during their initialization.
}

/**
 * @probeName
 * @hc
 */
PointLIFProbe::PointLIFProbe(const char * probeName, HyPerCol * hc) : PointProbe()
{
   initPointLIFProbe_base();
   initPointLIFProbe(probeName, hc);
}

int PointLIFProbe::initPointLIFProbe_base() {
   writeTime = 0.0;
   writeStep = 0.0;
   return PV_SUCCESS;
}

int PointLIFProbe::initPointLIFProbe(const char * probeName, HyPerCol * hc) {
   int status = initPointProbe(probeName, hc);
   writeTime = getParentCol()->getStartTime();
   return status;
}

int PointLIFProbe::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = PointProbe::ioParamsFillGroup(ioFlag);
   ioParam_writeStep(ioFlag);
   return status;
}

void PointLIFProbe::ioParam_writeStep(enum ParamsIOFlag ioFlag) {
   writeStep = getParentCol()->getDeltaTime();  // Marian, don't change this default behavior
   getParentCol()->ioParamValue(ioFlag, getProbeName(), "writeStep", &writeStep, writeStep, true/*warnIfAbsent*/);
}

/**
 * @time
 * @l
 * @k
 * @kex
 * NOTES:
 *     - Only the activity buffer covers the extended frame - this is the frame that
 * includes boundaries.
 *     - The other dynamic variables (G_E, G_I, V, Vth) cover the "real" or "restricted"
 *     frame.
 *     - sparseOutput was introduced to deal with ConditionalProbes.
 */
int PointLIFProbe::writeState(double timed, HyPerLayer * l, int k, int kex)
{
   assert(outputstream && outputstream->fp);
   LIF * LIF_layer = dynamic_cast<LIF *>(l);
   assert(LIF_layer != NULL);

   const pvdata_t * V = l->getV();
   const pvdata_t * activity = l->getLayerData();

   if (timed >= writeTime) {
      writeTime += writeStep;
      fprintf(outputstream->fp, "%s t=%.1f k=%d", msg, timed, k);
      pvdata_t * G_E  = LIF_layer->getConductance(CHANNEL_EXC);
      pvdata_t * G_I  = LIF_layer->getConductance(CHANNEL_INH);
      pvdata_t * G_IB = LIF_layer->getConductance(CHANNEL_INHB);
      pvdata_t * G_GAP = LIF_layer->getConductance(CHANNEL_GAP);
      pvdata_t * Vth  = LIF_layer->getVth();

      fprintf(outputstream->fp, " G_E=%6.3f", G_E[k]);
      fprintf(outputstream->fp, " G_I=%6.3f", G_I[k]);
      fprintf(outputstream->fp, " G_IB=%6.3f", G_IB[k]);
      if (G_GAP != NULL) { fprintf(outputstream->fp, " G_GAP=%6.3f", G_GAP[k]); }
      fprintf(outputstream->fp, " V=%6.3f", V[k]);
      fprintf(outputstream->fp, " Vth=%6.3f", Vth[k]);
      fprintf(outputstream->fp, " a=%.1f\n", activity[kex]);
      fflush(outputstream->fp);
   }
   return 0;
}

} // namespace PV
