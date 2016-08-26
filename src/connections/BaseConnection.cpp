/*
 * BaseConnection.hpp
 *
 *  Created on Sep 19, 2014
 *      Author: Pete Schultz
 *
 *  The abstract base class for the connection hierarchy.
 *  Only derived classes can be instantiated.
 *  The purpose is so that there can be a pointer to a conn without having
 *  to specify the specialization in the pointer declaration.
 */

#include <stdio.h>
#include <errno.h>
#include "BaseConnection.hpp"
#include <io/BaseConnectionProbe.hpp>

namespace PV {

BaseConnection::BaseConnection() {
   initialize_base();
}

int BaseConnection::initialize_base() {
   preLayerName = NULL;
   postLayerName = NULL;
   pre = NULL;
   post = NULL;
   channel = CHANNEL_EXC;
   delayArraySize = 0;
   this->fDelayArray = NULL;
   this->delays = NULL;
   numAxonalArborLists = 1;
   convertRateToSpikeCount = false;
#ifdef PV_USE_CUDA
   receiveGpu = false;
#endif //  PV_USE_CUDA
   this->initializeFromCheckpointFlag = false;
   this->probes = NULL;
   this->numProbes = 0;
   initInfoCommunicatedFlag = false;
   dataStructuresAllocatedFlag = false;
   initialValuesSetFlag = false;
   return PV_SUCCESS;
}

int BaseConnection::initialize(const char * name, HyPerCol * hc) {
   int status = BaseObject::initialize(name, hc);

   this->getParent()->addConnection(this);
   if (status == PV_SUCCESS) status = ioParams(PARAMS_IO_READ);
   return status;
}

int BaseConnection::setPreAndPostLayerNames() {
   return getPreAndPostLayerNames(this->getName(), &preLayerName, &postLayerName);
}

void BaseConnection::setPreLayerName(const char * pre_name) {
   assert(this->getParent()!=NULL);
   assert(this->preLayerName==NULL);
   if (pre_name != NULL) {
      this->preLayerName = strdup(pre_name);
      if (this->preLayerName==NULL) {
         pvError().printf("%s: rank %d process unable to allocate memory for name of presynaptic layer \"%s\": %s\n",
               getDescription_c(), this->getParent()->columnId(), pre_name, strerror(errno));
      }
   }
}

void BaseConnection::setPostLayerName(const char * post_name) {
   assert(this->postLayerName==NULL);
   if (post_name != NULL) {
      this->postLayerName = strdup(post_name);
      if (this->postLayerName==NULL) {
         pvError().printf("%s: rank %d process unable to allocate memory for name of postsynaptic layer \"%s\": %s\n",
               getDescription_c(), this->getParent()->columnId(), post_name, strerror(errno));
      }
   }
}

void BaseConnection::setPreSynapticLayer(HyPerLayer * pre) {
   assert(this->pre==NULL);
   if(pre != NULL){
      this->pre = pre;
   }
   else{
      if (parent->columnId()==0) {
         pvErrorNoExit().printf("%s: pre layer \"%s\" does not exist in the params file.\n", this->getDescription_c(), this->preLayerName);
      }
      MPI_Barrier(parent->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
}

void BaseConnection::setPostSynapticLayer(HyPerLayer * post) {
   assert(this->post==NULL);
   if(post != NULL){
      this->post = post;
   }
   else{
      if (parent->columnId()==0) {
         pvErrorNoExit().printf("%s: post layer \"%s\" does not exist in the params file.\n", this->getDescription_c(), this->postLayerName);
      }
      MPI_Barrier(parent->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
}

void BaseConnection::setChannelType(ChannelType ch) {
   assert(!initInfoCommunicatedFlag);
   this->channel = ch;
}

void BaseConnection::setNumberOfAxonalArborLists(int numArbors) {
   assert(!initInfoCommunicatedFlag);
   this->numAxonalArborLists = numArbors;
}

void BaseConnection::setConvertRateToSpikeCount(bool convertRateToSpikeCountFlag) {
   assert(!initInfoCommunicatedFlag);
   this->convertRateToSpikeCount = convertRateToSpikeCountFlag;
}

int BaseConnection::handleMissingPreAndPostLayerNames() {
   return inferPreAndPostFromConnName(this->getName(), this->getParent()->columnId(), &preLayerName, &postLayerName);
}

int BaseConnection::inferPreAndPostFromConnName(const char * name, int rank, char ** preLayerNamePtr, char ** postLayerNamePtr) {
   // If the connection name has the form "AbcToXyz", then *preLayerNamePtr will be Abc and *postLayerNamePtr will be Xyz.
   // If either of the intended pre- or post-layer names contains the string "To", this method cannot be used to infer them:
   // it returns PV_FAILURE if the string contains either more or less than one occurrence of the string "To", and does not change
   // *preLayerNamePtr or *postLayerNamePtr
   // This routine uses malloc to fill *{pre,post}LayerNamePtr, so the routine calling this one is responsible for freeing them.

   int status = PV_SUCCESS;
   // Check to see if the string "To" appears exactly once in name
   // If so, use part preceding "To" as pre-layer, and part after "To" as post.
   const char * separator = "To";
   const char * locto = strstr(name, separator);
   if( locto != NULL ) {
      const char * nextto = strstr(locto+1, separator); // Make sure "To" doesn't appear again.
      if( nextto == NULL ) {
         int seplen = strlen(separator);

         int pre_len = locto - name;
         *preLayerNamePtr = (char *) malloc((size_t) (pre_len + 1));
         if( *preLayerNamePtr==NULL) {
            pvError().printf("Connection \"%s\": unable to allocate memory for preLayerName: %s\n", name, strerror(errno));
         }
         const char * preInConnName = name;
         memcpy(*preLayerNamePtr, preInConnName, pre_len);
         (*preLayerNamePtr)[pre_len] = 0;

         int post_len = strlen(name)-pre_len-seplen;
         *postLayerNamePtr = (char *) malloc((size_t) (post_len + 1));
         if( *postLayerNamePtr==NULL) {
            pvError().printf("Connection \"%s\": unable to allocate memory for postLayerName: %s\n", name, strerror(errno));
         }
         const char * postInConnName = &name[pre_len+seplen];
         memcpy(*postLayerNamePtr, postInConnName, post_len);
         (*postLayerNamePtr)[post_len] = 0;
      }
      else {
         status = PV_FAILURE;
         if (rank==0) {
            pvErrorNoExit(errorMessage);
            errorMessage.printf("Unable to infer pre and post from connection name \"%s\":\n", name);
            errorMessage.printf("The string \"To\" cannot appear in the name more than once.\n");
         }
      }
   }
   else {
      status = PV_FAILURE;
      if (rank==0) {
         pvErrorNoExit(errorMessage);
         errorMessage.printf("Unable to infer pre and post from connection name \"%s\".\n", name);
         errorMessage.printf("The connection name must have the form \"AbcToXyz\", to infer the names,\n");
         errorMessage.printf("but the string \"To\" does not appear.\n");
      }
   }
   return status;
}

int BaseConnection::getPreAndPostLayerNames(const char * name, char ** preLayerNamePtr, char ** postLayerNamePtr) {
   // Retrieves preLayerName and postLayerName from parameter group whose name is given in the functions first argument.
   // This routine uses strdup to fill *{pre,post}LayerNamePtr, so the routine calling this one is responsible for freeing them.
   int status = PV_SUCCESS;
   *preLayerNamePtr = NULL;
   *postLayerNamePtr = NULL;
   if (preLayerName == NULL && postLayerName == NULL) {
      if (parent->getCommunicator()->commRank()==0) {
         pvInfo().printf("%s: preLayerName and postLayerName will be inferred in the communicateInitInfo stage.\n", getDescription_c());
      }
   }
   else if (preLayerName==NULL && postLayerName!=NULL) {
      status = PV_FAILURE;
      if (parent->getCommunicator()->commRank()==0) {
         pvErrorNoExit().printf("%s: if postLayerName is specified, preLayerName must be specified as well.\n", getDescription_c());
      }
   }
   else if (preLayerName!=NULL && postLayerName==NULL) {
      status = PV_FAILURE;
      if (parent->getCommunicator()->commRank()==0) {
         pvErrorNoExit().printf("%s: if preLayerName is specified, postLayerName must be specified as well.\n", getDescription_c());
      }
   }
   else {
      assert(preLayerName!=NULL && postLayerName!=NULL);
   }
   MPI_Barrier(parent->getCommunicator()->communicator());
   if (status != PV_SUCCESS) {
      exit(EXIT_FAILURE);
   }
   return status;
}

int BaseConnection::ioParams(enum ParamsIOFlag ioFlag) {
   parent->ioParamsStartGroup(ioFlag, this->getName());
   ioParamsFillGroup(ioFlag);
   parent->ioParamsFinishGroup(ioFlag);

   return PV_SUCCESS;
}

int BaseConnection::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = PV_SUCCESS;
   ioParam_preLayerName(ioFlag);
   ioParam_postLayerName(ioFlag);
   if (preLayerName == NULL || postLayerName == NULL) {
      status = setPreAndPostLayerNames();
   }
   ioParam_channelCode(ioFlag);
   ioParam_delay(ioFlag);
   ioParam_numAxonalArbors(ioFlag);
   ioParam_plasticityFlag(ioFlag);
   ioParam_convertRateToSpikeCount(ioFlag);

   // GPU-specific parameter.  If not using GPUs, we read it anyway, with warnIfAbsent set to false, to prevent unnecessary warnings from unread or missing parameters.
   ioParam_receiveGpu(ioFlag);

   return status;
}

void BaseConnection::ioParam_preLayerName(enum ParamsIOFlag ioFlag) {
   this->getParent()->ioParamString(ioFlag, this->getName(), "preLayerName", &preLayerName, NULL, false/*warnIfAbsent*/);
}

void BaseConnection::ioParam_postLayerName(enum ParamsIOFlag ioFlag) {
   this->getParent()->ioParamString(ioFlag, this->getName(), "postLayerName", &postLayerName, NULL, false/*warnIfAbsent*/);
}

void BaseConnection::ioParam_channelCode(enum ParamsIOFlag ioFlag) {
   if (ioFlag==PARAMS_IO_READ) {
      int ch = 0;
      this->getParent()->ioParamValueRequired(ioFlag, this->getName(), "channelCode", &ch);
      int status = decodeChannel(ch, &channel);
      if (status != PV_SUCCESS) {
         if (this->getParent()->columnId()==0) {
            pvErrorNoExit().printf("%s: channelCode %d is not a valid channel.\n",
                  this->getDescription_c(),  ch);
         }
         MPI_Barrier(this->getParent()->getCommunicator()->communicator());
         exit(EXIT_FAILURE);
      }
   }
   else if (ioFlag==PARAMS_IO_WRITE) {
      int ch = (int) channel;
      this->getParent()->ioParamValueRequired(ioFlag, this->getName(), "channelCode", &ch);
   }
   else {
      assert(0); // All possibilities of ioFlag are covered above.
   }
}

void BaseConnection::ioParam_delay(enum ParamsIOFlag ioFlag) {
   //Grab delays in ms and load into fDelayArray.
   //initializeDelays() will convert the delays to timesteps store into delays.
   this->getParent()->ioParamArray(ioFlag, this->getName(), "delay", &fDelayArray, &delayArraySize);
   if (ioFlag==PARAMS_IO_READ && delayArraySize==0) {
      assert(fDelayArray==NULL);
      fDelayArray = (float *) malloc(sizeof(float));
      if (fDelayArray == NULL) {
         pvError().printf("%s: unable to set default delay: %s\n",
               this->getDescription_c(), strerror(errno));
      }
      *fDelayArray = 0.0f; // Default delay
      delayArraySize = 1;
      if (this->getParent()->columnId()==0) {
         pvInfo().printf("%s: Using default value of zero for delay.\n",
               this->getDescription_c());
      }
   }
}

void BaseConnection::ioParam_numAxonalArbors(enum ParamsIOFlag ioFlag) {
   int numArbors = this->numberOfAxonalArborLists();
   this->getParent()->ioParamValue(ioFlag, this->getName(), "numAxonalArbors", &numArbors, 1);
   if (ioFlag == PARAMS_IO_READ) {
      this->setNumberOfAxonalArborLists(numArbors);
      if (ioFlag == PARAMS_IO_READ && this->numberOfAxonalArborLists()==0 && this->getParent()->columnId()==0) {
         pvWarn().printf("Connection %s: Variable numAxonalArbors is set to 0. No connections will be made.\n", this->getName());
      }
   }
}

void BaseConnection::ioParam_plasticityFlag(enum ParamsIOFlag ioFlag) {
   parent->ioParamValue(ioFlag, name, "plasticityFlag", &plasticityFlag, true/*default value*/);
}

// preActivityIsNotRate was replaced with convertRateToSpikeCount on Dec 31, 2014.
// The warning issued if the params file contained preActivityIsNotRate was removed on Aug 5, 2014.

void BaseConnection::ioParam_convertRateToSpikeCount(enum ParamsIOFlag ioFlag) {
   getParent()->ioParamValue(ioFlag, this->getName(), "convertRateToSpikeCount", &convertRateToSpikeCount, false/*default value*/);
}

void BaseConnection::ioParam_receiveGpu(enum ParamsIOFlag ioFlag) {
#ifdef PV_USE_CUDA
   parent->ioParamValue(ioFlag, name, "receiveGpu", &receiveGpu, false/*default*/, true/*warn if absent*/);
#else
   bool receiveGpu = false;
   parent->ioParamValue(ioFlag, name, "receiveGpu", &receiveGpu, receiveGpu/*default*/, false/*warn if absent*/);
   if (ioFlag==PARAMS_IO_READ && receiveGpu) {
      if (parent->columnId()==0) {
         pvErrorNoExit().printf("%s: receiveGpu is set to true, but PetaVision was compiled without GPU acceleration.\n",
               this->getDescription_c());
      }
      MPI_Barrier(parent->getCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
#endif // PV_USE_CUDA
}


void BaseConnection::ioParam_initializeFromCheckpointFlag(enum ParamsIOFlag ioFlag) {
   assert(parent->getInitializeFromCheckpointDir()); // If we're not initializing any layers or connections from a checkpoint, this should be the empty string, not null.
   if (parent->getInitializeFromCheckpointDir() && parent->getInitializeFromCheckpointDir()[0]) {
      parent->ioParamValue(ioFlag, name, "initializeFromCheckpointFlag", &initializeFromCheckpointFlag, parent->getDefaultInitializeFromCheckpointFlag(), true/*warnIfAbsent*/);
   }
}

int BaseConnection::insertProbe(BaseConnectionProbe * p)
{
   if(p->getTargetConn() != this) {
      pvWarn().printf("%s: insertProbe called with probe %p, whose targetConn is not this connection.  Probe was not inserted.\n", getDescription_c(), p);
      return numProbes;
   }
   for( int i=0; i<numProbes; i++ ) {
      if( p == probes[i] ) {
         pvWarn().printf("%s: insertProbe called with probe %p, which has already been inserted as probe %d.\n", getDescription_c(), p, i);
         return numProbes;
      }
   }

   BaseConnectionProbe ** tmp;
   tmp = (BaseConnectionProbe **) malloc((numProbes + 1) * sizeof(BaseConnectionProbe *));
   assert(tmp != NULL);

   for (int i = 0; i < numProbes; i++) {
      tmp[i] = probes[i];
   }
   delete probes;

   probes = tmp;
   probes[numProbes] = p;

   return ++numProbes;
}
int BaseConnection::respond(std::shared_ptr<BaseMessage> message) {
   int status = BaseObject::respond(message);
   if (status != PV_SUCCESS) {
      return status;
   }
   else if (ConnectionUpdateMessage const * castMessage = dynamic_cast<ConnectionUpdateMessage const*>(message.get())) {
      return respondConnectionUpdate(castMessage);
   }
   else if (ConnectionFinalizeUpdateMessage const * castMessage = dynamic_cast<ConnectionFinalizeUpdateMessage const*>(message.get())) {
      return respondConnectionFinalizeUpdate(castMessage);
   }
   else if (ConnectionOutputMessage const * castMessage = dynamic_cast<ConnectionOutputMessage const*>(message.get())) {
      return respondConnectionOutput(castMessage);
   }
   else {
      return status;
   }
}

int BaseConnection::outputProbeParams() {
   int status = PV_SUCCESS;
   for (int p=0; p<numProbes; p++) {
      int status1 = probes[p]->ioParams(PARAMS_IO_WRITE);
      if (status1 != PV_SUCCESS) { status = PV_FAILURE; }
   }
   return status;
}

int BaseConnection::communicateInitInfo() {
   int status = PV_SUCCESS;

   if (this->getPreLayerName()==NULL) {
      assert(this->getPostLayerName()==NULL);
      status = handleMissingPreAndPostLayerNames();
   }
   MPI_Barrier(this->getParent()->getCommunicator()->communicator());
   if (status != PV_SUCCESS) {
      assert(this->getPreLayerName()==NULL && this->getPostLayerName()==NULL);
      if (this->getParent()->columnId()==0) {
         pvErrorNoExit().printf("%s: Unable to determine pre- and post-layer names.  Exiting.\n", this->getDescription_c());
      }
      exit(EXIT_FAILURE);
   }
   this->setPreSynapticLayer(this->getParent()->getLayerFromName(this->getPreLayerName()));
   this->setPostSynapticLayer(this->getParent()->getLayerFromName(this->getPostLayerName()));
   if (this->preSynapticLayer()==NULL) {
      if (this->getParent()->columnId()==0) {
         pvErrorNoExit().printf("%s: preLayerName \"%s\" does not correspond to a layer in the column.\n", getDescription_c(), this->getPreLayerName());
      }
      status = PV_FAILURE;
   }
   if (this->postSynapticLayer()==NULL) {
      if (this->getParent()->columnId()==0) {
         pvErrorNoExit().printf("%s: postLayerName \"%s\" does not correspond to a layer in the column.\n", getDescription_c(), this->getPostLayerName());
      }
      status = PV_FAILURE;
   }
   MPI_Barrier(this->getParent()->getCommunicator()->communicator());
   if (status != PV_SUCCESS) {
      exit(EXIT_FAILURE);
   }

   initializeDelays(fDelayArray, delayArraySize);
   int maxDelay = maxDelaySteps();
   int allowedDelay = this->preSynapticLayer()->increaseDelayLevels(maxDelay);
   if( allowedDelay < maxDelay ) {
      if( this->getParent()->columnId() == 0 ) {
         pvErrorNoExit().printf("%s: attempt to set delay to %d, but the maximum allowed delay is %d.  Exiting\n", getDescription_c(), maxDelay, allowedDelay);
      }
      exit(EXIT_FAILURE);
   }

   // Make sure post-synaptic layer has enough channels.
   int num_channels_check = 0;
   int ch = (int) this->getChannel();
   if (ch>=0) {
      status = this->postSynapticLayer()->requireChannel(ch, &num_channels_check);
   }
   assert(status != PV_SUCCESS || num_channels_check > (int) this->getChannel()); // if requireChannel passes, layer's numChannels should be large enough for the connection's channel
   if (status != PV_SUCCESS) {
      if (parent->columnId()==0) {
         pvErrorNoExit().printf("%s: postsynaptic layer \"%s\" failed to add channel %d\n",
               this->getDescription_c(), this->postSynapticLayer()->getName(), (int) this->getChannel());
      }
   }
   return status;
}

int BaseConnection::initializeDelays(const float * fDelayArray, int size){
   int status = PV_SUCCESS;
   assert(!this->getParent()->parameters()->presentAndNotBeenRead(this->getName(), "numAxonalArbors"));
   //Allocate delay data structure
   delays = (int *) calloc(this->numberOfAxonalArborLists(), sizeof(int));
   if( delays == NULL ) {
      pvError().printf("%s: unable to allocate memory for %d delays: %s\n", getDescription_c(), size, strerror(errno));
   }

   //Initialize delays for each arbor
   //Using setDelay to convert ms to timesteps
   for (int arborId=0;arborId<this->numberOfAxonalArborLists();arborId++) {
      if (size == 0){
         //No delay
         setDelay(arborId, 0);
      }
      else if (size == 1){
         setDelay(arborId, fDelayArray[0]);
      }
      else if (size == this->numberOfAxonalArborLists()){
         setDelay(arborId, fDelayArray[arborId]);
      }
      else{
         pvError().printf("Delay must be either a single value or the same length as the number of arbors\n");
      }
   }
   return status;
}

int BaseConnection::maxDelaySteps() {
   int maxDelay = 0;
   for (int arborId=0; arborId<numberOfAxonalArborLists(); arborId++) {
      if (delays[arborId]>maxDelay) { maxDelay = delays[arborId]; }
   }
   return maxDelay;
}

//Input delay is in ms
void BaseConnection::setDelay(int arborId, float delay) {
   assert(arborId>=0 && arborId<this->numberOfAxonalArborLists());
   int intDelay = round(delay/this->getParent()->getDeltaTime());
   if (fmod(delay, this->getParent()->getDeltaTime()) != 0){
      float actualDelay = intDelay * this->getParent()->getDeltaTime();
      pvWarn() << this->getName() << ": A delay of " << delay << " will be rounded to " << actualDelay << "\n";
   }
   delays[arborId] = (int)(intDelay);
}

int BaseConnection::initializeState() {
   int status = PV_SUCCESS;
   assert(parent->getInitializeFromCheckpointDir()); // should never be null; it should be the empty string if not initializing from a checkpoint
   if (!this->getPlasticityFlag() && parent->getSuppressNonplasticCheckpoints()) {
      status = setInitialValues();
   }
   else if (parent->getCheckpointReadFlag()) {
      double checkTime = parent->simulationTime();
      checkpointRead(parent->getCheckpointReadDir(), &checkTime);
   }
   else if (initializeFromCheckpointFlag) {
      assert(parent->getInitializeFromCheckpointDir() && parent->getInitializeFromCheckpointDir()[0]);
      status = readStateFromCheckpoint(parent->getInitializeFromCheckpointDir(), NULL);
   }
   else {
      //initialize weights for patches:
      status = setInitialValues();
   }
   return status;
}

BaseConnection::~BaseConnection() {
   free(this->preLayerName);
   free(this->postLayerName);
   free(fDelayArray);
   free(delays);
   free(this->probes); // All probes are deleted by the HyPerCol, so probes[i] doesn't need to be deleted, only the array itself.

}

}  // end namespace PV
