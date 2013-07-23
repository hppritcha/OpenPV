/*
 * GapLayer.hpp
 * can be used to implement gap junctions
 *
 *  Created on: May 11, 2011
 *      Author: garkenyon
 */

#ifndef GAPLAYER_HPP_
#define GAPLAYER_HPP_

#include "HyPerLayer.hpp"
#include "LIFGap.hpp"

namespace PV {

// CloneLayer can be used to implement gap junctions between spiking neurons
class GapLayer: public HyPerLayer {
public:
   GapLayer(const char * name, HyPerCol * hc, const char * originalLayerName);
   virtual ~GapLayer();

   virtual int communicateInitInfo();
   virtual int allocateDataStructures();

   virtual int updateState(double timef, double dt);

   // virtual int updateV();

protected:
   GapLayer();
   int initialize(const char * name, HyPerCol * hc, const char * originalLayerName);
      // use LIFGap as source layer instead (LIFGap updates gap junctions more accurately)

   /* static */ int updateState(double timef, double dt, const PVLayerLoc * loc, pvdata_t * A, pvdata_t * V, pvdata_t * checkActive);
   virtual int setActivity();

private:
   int initialize_base();

   char * sourceLayerName;
   LIFGap * sourceLayer;
   float ampSpikelet;

};

}

#endif /* GAPLAYER_HPP_ */
