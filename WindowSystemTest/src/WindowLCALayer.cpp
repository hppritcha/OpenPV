/*
 * WindowLCALayera.cpp
 *
 *  Created on: Jan 24, 2013
 *      Author: garkenyon
 */

#include "WindowLCALayer.hpp"
#include <layers/ANNLayer.cpp>

namespace PV {

WindowLCALayer::WindowLCALayer(const char * name, HyPerCol * hc)
{
   int numChannels = 2;
   HyPerLCALayer::initialize(name, hc);
}

WindowLCALayer::~WindowLCALayer()
{
}

//Overwrite LCA to ANNLayer's updateState
int WindowLCALayer::doUpdateState(double time, double dt, const PVLayerLoc * loc, pvdata_t * A,
      pvdata_t * V, int num_channels, pvdata_t * gSynHead){
   int status = ANNLayer::doUpdateState(time, dt, loc, A, V, num_channels, gSynHead);
   return status;
}

}
