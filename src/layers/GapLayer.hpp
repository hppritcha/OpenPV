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
   GapLayer(const char * name, HyPerCol * hc, LIFGap * clone);
   virtual ~GapLayer();

   virtual int updateState(float timef, float dt);

   // virtual int updateV();

   // virtual int setActivity();
   LIFGap * sourceLayer;

protected:
   GapLayer();
   int initialize(const char * name, HyPerCol * hc, LIFGap * originalLayer);
      // use LIFGap as source layer instead (LIFGap updates gap junctions more accurately)

   /* static */ int updateState(float timef, float dt, const PVLayerLoc * loc, pvdata_t * A, pvdata_t * V, const PVLayerLoc * src_loc, bool src_spiking, unsigned int src_num_active, unsigned int * src_active_indices);

private:
   int initialize_base();

};

}

#endif /* GAPLAYER_HPP_ */
