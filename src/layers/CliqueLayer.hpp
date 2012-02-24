/*
 * CliqueLayer.h
 *
 *  Created on: Sep 3, 2011
 *      Author: gkenyon
 */

#ifndef CLIQUELAYER_H_
#define CLIQUELAYER_H_

#include "../columns/HyPerCol.hpp"
#include "../connections/HyPerConn.hpp"
#include "ANNLayer.hpp"


namespace PV {

class CliqueLayer: public PV::ANNLayer {
public:
   CliqueLayer(const char * name, HyPerCol * hc, int numChannels);
   CliqueLayer(const char * name, HyPerCol * hc);
   virtual ~CliqueLayer();
   virtual int recvSynapticInput(HyPerConn * conn, const PVLayerCube * activity, int axonId);
   virtual int updateState(float time, float dt);
   virtual int updateActiveIndices();
protected:
   CliqueLayer();
   int initialize(const char * name, HyPerCol * hc, int numChannels);
   pvdata_t Vgain;
   pvdata_t Voffset;
   int cliqueSize; // number of presynaptic cells in clique (traditional ANN uses 1)
private:
   int initialize_base();
};

} /* namespace PV */

#endif /* CLIQUELAYER_H_ */
