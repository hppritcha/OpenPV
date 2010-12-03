/*
 * HyPerLayer.hpp
 *
 *  Created on: Aug 3, 2008
 *      Author: dcoates
 */

#ifndef HYPERLAYER_HPP_
#define HYPERLAYER_HPP_

#include "../layers/PVLayer.h"
#include "../layers/LayerDataInterface.hpp"
#include "../layers/LIF2.h"
#include "../columns/DataStore.hpp"
#include "../columns/HyPerCol.hpp"
#include "../columns/InterColComm.hpp"
#include "../io/LayerProbe.hpp"
#include "../include/pv_types.h"

#include "../arch/opencl/CLBuffer.hpp"

namespace PV {

// HyPerLayer uses C code from PVLayer.{h,c}, and LIF2.{h,c}
typedef LIF2_params HyPerLayerParams;

/**
 * OpenCLLayer collects memory objects for sharing data with OpenCL devices
 */
typedef struct {
   CLBuffer * V;
   CLBuffer * G_E;
   CLBuffer * G_I;
   CLBuffer * G_IB;
   CLBuffer * phi;   // collects all three phi buffers and hopefully will go away
   CLBuffer * activity;
} OpenCLLayer;


class HyPerLayer : public LayerDataInterface {

protected:

   // only subclasses can be constructed directly
   HyPerLayer(const char * name, HyPerCol * hc);

private:
   int initialize_base(const char * name, HyPerCol * hc);
   int initializeThreads();

public:

   virtual ~HyPerLayer() = 0;

   static int copyToBuffer(pvdata_t * buf, const pvdata_t * data,
                           const PVLayerLoc * loc, bool extended, float scale);
   static int copyToBuffer(unsigned char * buf, const pvdata_t * data,
                           const PVLayerLoc * loc, bool extended, float scale);

   static int copyFromBuffer(const pvdata_t * buf, pvdata_t * data,
                             const PVLayerLoc * loc, bool extended, float scale);
   static int copyFromBuffer(const unsigned char * buf, pvdata_t * data,
                             const PVLayerLoc * loc, bool extended, float scale);

   // TODO - make protected
   PVLayer*  clayer;
   HyPerCol* parent;

   virtual int updateState(float time, float dt) = 0;

   virtual int
       recvSynapticInput(HyPerConn * conn, PVLayerCube * cube, int neighbor);

   virtual int reconstruct(HyPerConn * conn, PVLayerCube * cube);

   int initialize(PVLayerType type);
   int initFinish();

   PVLayerCube * initBorder(PVLayerCube * border, int borderId);

   int mirrorInteriorToBorder(int whichBorder, PVLayerCube * cube, PVLayerCube * borderCube);

   virtual int columnWillAddLayer(InterColComm * comm, int id);

   virtual int setParams(int numParams, size_t sizeParams, float * params);
   virtual int getParams(int * numParams, float ** params);
   virtual int setFuncs(void * initFunc, void * updateFunc);

   virtual int publish(InterColComm * comm, float time);
   virtual int outputState(float time, bool last=false);
   virtual int writeState(const char * name, float time, bool last=false);
   virtual int writeActivity(const char * filename, float time);
   virtual int writeActivity(float time);
   virtual int writeActivitySparse(float time);
   virtual int readState(const char * name, float * time);

   virtual int insertProbe(LayerProbe * probe);

   /** returns the number of neurons in layer (for borderId=0) or a border region **/
   virtual int numberOfNeurons(int borderId);

   virtual int mirrorToNorthWest(PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToNorth    (PVLayerCube * dest, PVLayerCube* src);
   virtual int mirrorToNorthEast(PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToWest     (PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToEast     (PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToSouthWest(PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToSouth    (PVLayerCube * dest, PVLayerCube * src);
   virtual int mirrorToSouthEast(PVLayerCube * dest, PVLayerCube * src);

   // Public access functions:

   const char * getName()            {return name;}

   int getNumNeurons()               {return clayer->numNeurons;}
   int getNumExtended()              {return clayer->numExtended;}

   int  getLayerId()                 {return clayer->layerId;}
   PVLayerType getLayerType()        {return clayer->layerType;}
   void setLayerId(int id)           {clayer->layerId = id;}

   PVLayer*  getCLayer()             {return clayer;}
   pvdata_t * getV()                 {return clayer->V;}           // name query
   pvdata_t * getChannel(ChannelType ch) {                         // name query
      return ch < clayer->numPhis ? clayer->phi[ch] : NULL;
   }
   int getXScale()                   {return clayer->xScale;}
   int getYScale()                   {return clayer->yScale;}

   HyPerCol* getParent()             {return parent;}
   void setParent(HyPerCol* parent)  {this->parent = parent;}

   bool useMirrorBCs()               {return this->mirrorBCflag;}

   // implementation of LayerDataInterface interface
   //
   const pvdata_t   * getLayerData();
   const PVLayerLoc * getLayerLoc()  { return &clayer->loc; }
   bool isExtended()                 { return true; }

   virtual int gatherToInteriorBuffer(unsigned char * buf);

protected:
   virtual int initGlobal(int colId, int colRow, int colCol, int nRows, int nCols);

   char * name;  // well known name of layer

   int numProbes;
   LayerProbe ** probes;

   bool mirrorBCflag;           // true when mirror BC are to be applied

   int ioAppend;                // controls opening of binary files
   float writeTime;             // time of next output
   float writeStep;             // output time interval

   OpenCLLayer clBuffers;       // data shared with OpenCL devices
};

} // namespace PV

#endif /* HYPERLAYER_HPP_ */
