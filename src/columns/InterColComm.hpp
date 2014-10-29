/*
 * InterColComm.h
 *
 *  Created on: Aug 28, 2008
 *      Author: rasmussn
 */

#ifndef INTERCOLCOMM_HPP_
#define INTERCOLCOMM_HPP_

#include "Communicator.hpp"
#include "DataStore.hpp"
#include "../include/pv_common.h"

// maximum number of messages (each layer publishes to all neighbors)
#define MAX_MESSAGES    MAX_NEIGHBORS
// #define MAX_PUBLISHERS  MAX_LAYERS
// #define MAX_SUBSCRIBERS MAX_LAYERS

namespace PV {

class HyPerCol;
class HyPerLayer;
class BaseConnection;

class Publisher {

public:
//#ifdef PV_USE_OPENCL
//   Publisher(int pubId, HyPerCol * hc, int numItems, PVLayerLoc loc, int numLevels, bool copydstoreflag);
//#else
   Publisher(int pubId, HyPerCol * hc, int numItems, PVLayerLoc loc, int numLevels, bool isSparse);
//#endif
   virtual ~Publisher();
   int readData(int delay);
   int publish(HyPerLayer * pub, int neighbors[], int numNeighbors,
               int borders[], int numBorders, PVLayerCube * data,
               unsigned int * activeIndicies, unsigned int numActive,
               int delay=0);
   int subscribe(BaseConnection * conn);
   int exchangeBorders(int neighbors[], int numNeighbors, const PVLayerLoc * loc, int delay=0);
   int wait();

   static int borderStoreIndex(int i, int numNeighbors)  {return i+numNeighbors;}

   int increaseTimeLevel()   {return store->newLevelIndex();}

   DataStore * dataStore()   {return store;}

private:

   pvdata_t * recvBuffer(int bufferId)
         {return (pvdata_t *) store->buffer(bufferId);}
   pvdata_t * recvBuffer(int bufferId, int delay)
         {return (pvdata_t *) store->buffer(bufferId, delay);}

   unsigned int * recvNumActiveBuffer(int bufferId){
      return store->numActiveBuffer(bufferId);
   }
   unsigned int * recvNumActiveBuffer(int bufferId, int delay){
      return store->numActiveBuffer(bufferId, delay);
   }

   unsigned int * recvActiveIndiciesBuffer(int bufferId){
      return store->activeIndiciesBuffer(bufferId);
   }
   unsigned int * recvActiveIndiciesBuffer(int bufferId, int delay){
      return store->activeIndiciesBuffer(bufferId, delay);
   }

   int pubId;
   int numSubscribers;
   int subscriberArraySize;
   BaseConnection ** connection;
   DataStore * store;

   PVLayerCube cube;

   Communicator * comm;

   int            numRequests;
   MPI_Request    requests[NUM_NEIGHBORHOOD-1];
   MPI_Datatype * neighborDatatypes;
};

class InterColComm : public Communicator {

public:
   InterColComm(int * argc, char *** argv);
   virtual ~InterColComm();

   int addPublisher(HyPerLayer * pub, int numItems, int numLevels, bool isSparse);
   int clearPublishers();
   int publish(HyPerLayer * pub, PVLayerCube * cube, unsigned int * activeIndicies, unsigned int numActive);
   int subscribe(BaseConnection * conn);
   int exchangeBorders(int pubId, const PVLayerLoc * loc, int delay=0);
   int wait(int pubId);

   int increaseTimeLevel(int pubId)        {return publishers[pubId]->increaseTimeLevel();}

   DataStore * publisherStore(int pubId)   {return publishers[pubId]->dataStore();}

private:

   int numPublishers;
   int publisherArraySize;
   Publisher ** publishers;
   int resizePublishersArray(int newSize);
};

} // namespace PV

#endif /* INTERCOLCOMM_HPP_ */
