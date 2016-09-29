/*
 * Secretary.hpp
 *
 *  Created on Sep 28, 2016
 *      Author: Pete Schultz
 */

#ifndef SECRETARY_HPP_
#define SECRETARY_HPP_

#include "observerpattern/Subject.hpp"
#include "columns/Communicator.hpp"
#include "io/CheckpointEntry.hpp"
#include "io/io.hpp"
#include "io/PVParams.hpp"
#include "observerpattern/BaseMessage.hpp"
#include <ctime>
#include <map>
#include <memory>

namespace PV {

class Secretary : public Subject {
public:
   struct TimeInfo {
      double mSimTime = 0.0;
      long int mCurrentCheckpointStep = -1L; // increments at start of each checkpoint; the first time checkpointWrite the step number will be zero.
   };
   class ProcessCheckpointReadMessage : public BaseMessage {
   public:
      ProcessCheckpointReadMessage() {
         setMessageType("PrepareCheckpointReadMessage");
      }
   };
   class PrepareCheckpointWriteMessage : public BaseMessage {
   public:
      PrepareCheckpointWriteMessage() {
         setMessageType("ProcessCheckpointWriteMessage");
      }
   };
   Secretary(std::string const& name, Communicator * comm);
   ~Secretary();

   void setOutputPath(std::string const& outputPath);
   void ioParamsFillGroup(enum ParamsIOFlag ioFlag, PVParams * params);
   void provideFinalStep(long int finalStep);
   bool registerCheckpointEntry(std::shared_ptr<CheckpointEntry> checkpointEntry);
   virtual void addObserver(Observer * observer, BaseMessage const& message) override;
   void checkpointRead(std::string const& checkpointReadDir, double * simTimePointer, long int * currentStepPointer);
   void checkpointWrite(double simTime);
   void finalCheckpoint(double simTime);

   Communicator * getCommunicator() { return mCommunicator; }
   bool doesVerifyWrites() { return mVerifyWritesFlag; }
   char const * getOutputPath() { return mOutputPath; }
private:
   enum CheckpointWriteTriggerMode {NONE, STEP, SIMTIME, WALLCLOCK};
   enum WallClockUnit {SECOND, MINUTE, HOUR, DAY};
   void ioParam_verifyWrites(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_outputPath(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointWrite(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointWriteDir(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointWriteTriggerMode(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointWriteStepInterval(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointWriteTimeInterval(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_checkpointIndexWidth(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_suppressNonplasticCheckpoints(enum ParamsIOFlag ioFlag, PVParams * params);
   void ioParam_suppressLastOutput(enum ParamsIOFlag ioFlag, PVParams * params);
   bool checkpointWriteSignal();
   void checkpointWriteStep();
   void checkpointWriteSimtime();
   void checkpointWriteWallclock();
   void checkpointNow();
   void checkpointToDirectory(std::string const& checkpointDirectory);

private:
   std::string mName;
   Communicator * mCommunicator = nullptr;
   std::vector<std::shared_ptr<CheckpointEntry> > mCheckpointRegistry; // Needs to be a vector so that each MPI process iterates over the entries in the same order.
   ObserverTable mObserverTable;
   TimeInfo mTimeInfo;
   std::shared_ptr<CheckpointEntryData<TimeInfo> > mTimeInfoCheckpointEntry = nullptr; 
   bool mVerifyWritesFlag = true;
   char * mOutputPath = nullptr;
   bool mCheckpointWriteFlag = false;
   char * mCheckpointWriteDir = nullptr;
   char * mCheckpointWriteTriggerModeString = nullptr;
   enum CheckpointWriteTriggerMode mCheckpointWriteTriggerMode = NONE;
   long int mCheckpointWriteStepInterval = 1L;
   double mCheckpointWriteSimtimeInterval = 1.0;
   double mCheckpointWriteWallclockInterval = 1.0;
   int mCheckpointIndexWidth = -1;
   bool mSuppressNonplasticCheckpoints = false;
   bool mSuppressLastOutput = false;
   std::string mCheckpointReadDirectory;
   int mCheckpointSignal = 0;
   double mLastCheckpointSimtime = -std::numeric_limits<double>::infinity();
   std::time_t mLastCheckpointWallclock = (std::time_t) 0;
   int mWidthOfFinalStepNumber = 0;

   static std::string const mDefaultOutputPath;
};

/**
 * SecretaryDataInterface provides a virtual method intended for interfacing
 * with Secretary register methods.  An object that does checkpointing should
 * derive from SecretaryDataInterface, and override registerData to call
 * Secretary::registerCheckpointEntry once for each piece of checkpointable
 * data.
 *
 * BaseObject derives from SecretaryDataInterface, and calls registerData
 * when it receives a RegisterDataMessage (which HyPerCol::run calls after
 * AllocateDataMessage and before InitializeStateMessage).
 */
class SecretaryDataInterface {
public:
   virtual int registerData(Secretary * secretary, std::string const& objName) { return PV_SUCCESS; }
};

}  // namespace PV

#endif // SECRETARY_HPP_