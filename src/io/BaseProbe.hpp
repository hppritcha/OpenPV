/*
 * BaseProbe.hpp
 *
 *      Author: slundquist
 */

#ifndef BASEPROBE_HPP_
#define BASEPROBE_HPP_

#include <stdio.h>
#include <vector>
#include "include/pv_common.h"
#include "columns/BaseObject.hpp"
#include "io/FileStream.hpp"
#include "io/io.hpp"

namespace PV {

class HyPerCol;
class HyPerLayer;

//typedef enum {
//   BufV,
//   BufActivity
//} PVBufType;

/**
 * An abstract base class for the common functionality of layer probes and connection probes.
 */
class BaseProbe : public BaseObject {

// Methods
public:
   //BaseProbe(const char * probeName, HyPerCol * hc);
   virtual ~BaseProbe();

   int ioParams(enum ParamsIOFlag ioFlag);

   /**
    * A pure virtual function called during HyPerCol::run, during the communicateInitInfo stage.
    * BaseProbe::communicateInitInfo sets up the triggering layer and attaches to the energy probe,
    * if either triggerFlag or energyProbe are set.
    */
   virtual int communicateInitInfo() = 0;

   /**
    * Called during HyPerCol::run, during the allocateDataStructures stage.
    * BaseProbe::allocateDataStructures sets up the output stream.
    * Derived classes that override this method should make sure to
    * call this method in their own allocateDataStructures methods.
    */
   virtual int allocateDataStructures();

   /**
    * If there is a triggering layer, needUpdate returns true when the triggering layer's
    * nextUpdateTime, modified by the probe's triggerOffset parameter, occurs; otherwise false.
    * If there is not a triggering layer, needUpdate always returns true.
    * This behavior can be overridden if a probe uses some criterion other than triggering
    * to choose when output its state.
    */
   virtual bool needUpdate(double time, double dt);

   /**
    * Returns the number of value indices the probe can compute (typically the value
    * of the parent HyPerCol's nBatch parameter).
    * BaseProbe::getNumValues() returns the parent HyPerCol's getNBatch(), which can be overridden.
    * Probes derived from BaseProbe can set numValues to zero or a negative number to indicate that
    * getValues() and getNumValues()
    * are not fully implemented for that probe.
    */
   int getNumValues() { return numValues; }
   
   /**
    * The public interface for calling the outputState method.
    * BaseConnection::outputStateWrapper calls outputState() if needUpdate() returns true.
    * This behavior is intended to be general, but the method can be overridden if needed.
    */
   virtual int outputStateWrapper(double timef, double dt);
   
   /**
    * A pure virtual method for writing output to the output file.
    */
   virtual int outputState(double timef) = 0;
   virtual int writeTimer(std::ostream& stream) {return PV_SUCCESS;}
   
   /**
    * Returns the name of the targetName parameter for this probe.
    * LayerProbe uses targetName to specify the layer to attach to;
    * BaseConnectionProbe uses it to specify the connection to attach to.
    */
   const char * getTargetName() {return targetName;}

   /**
    * Returns the name of the energy probe the probe is attached to (null if not attached to an energy probe)
    */
   char const * getEnergyProbe() { return energyProbe; }
   
   /**
    * Returns the coefficient if the energy probe is set.
    */
   double getCoefficient() { return coefficient; }

   /**
    * getValues(double timevalue, double * values) sets the buffer 'values' with the probe's calculated values.
    * It assumes that the values buffer is large enough to hold getNumValues()
    * double-precision values.
    * If 'values' is NULL, the values are still updated internally if needed, but
    * those values are not returned.
    * Internally, getValues() calls calcValues() if needRecalc() is true.  It then
    * copies the probeValues buffer to the input argument buffer 'values'.
    * Derived classes should not override or hide this method.  Instead, they should override calcValues.
    */
   int getValues(double timevalue, double * valuesVector);
   /**
    * getValues(double timevalue, vector<double> * valuesVector) is a wrapper around
    * getValues(double, double *) that uses C++ vectors.  It resizes valuesVector
    * to size getNumValues() and then fills the vector with the values returned by getValues.
    */
   int getValues(double timevalue, std::vector<double> * valuesVector);
   /**
    * getValue() is meant for situations where the caller needs one value
    * that would be returned by getValues(), not the whole buffer.
    * getValue() returns a signaling NaN if index is out of bounds.  If index is valid,
    * getValue() calls calcValues() if needRecalc() returns true, and then
    * returns probeValues[index].
    * Derived classes should not override or hide this method.  Instead, they should override calcValues.
    */
   double getValue(double timevalue, int index);

protected:
   BaseProbe();
   int initialize(const char * probeName, HyPerCol * hc);
   virtual int ioParamsFillGroup(enum ParamsIOFlag ioFlag);

   /** 
    * List of parameters for the BaseProbe class
    * @name BaseProbe Parameters
    * @{
    */

   /**
    * @brief targetName: the name of the object that the probe attaches to.
    * In LayerProbe, targetName is used to define the targetLayer, and in
    * BaseConnectionProbe, targetName is used to define the targetConn.
    */
   virtual void ioParam_targetName(enum ParamsIOFlag ioFlag);
   
   /**
    * @brief message: A string parameter that is typically included in the lines output by the outputState method
    */
   virtual void ioParam_message(enum ParamsIOFlag ioFlag);

   /**
    * @brief textOutputFlag: A boolean parameter that sets whether to generate an output file.  Defaults to true.
    */
   virtual void ioParam_textOutputFlag(enum ParamsIOFlag ioFlag);

   /**
    * @brief probeOutputFile: If textOutputFlag is true, probeOutputFile specifies
    * the name of the file that the outputState method writes to.
    * If blank, the output is sent to the output stream.
    */
   virtual void ioParam_probeOutputFile(enum ParamsIOFlag ioFlag);

   /**
    * @brief triggerFlag: If false, the needUpdate method always returns true,
    * so that outputState is called every timestep.  If true, the needUpdate
    * method uses triggerLayerName and triggerOffset to determine if the probe
    * should be triggered.
    */
   virtual void ioParam_triggerFlag(enum ParamsIOFlag ioFlag);
   
   /**
    * @brief triggerLayerName: If triggerFlag is true, triggerLayerName specifies the layer
    * to check for triggering.
    */
   virtual void ioParam_triggerLayerName(enum ParamsIOFlag ioFlag);
   
   /**
    * @brief triggerOffset: If triggerFlag is true, triggerOffset specifies the
    * time interval *before* the triggerLayer's nextUpdate time that needUpdate() returns true.
    */
   virtual void ioParam_triggerOffset(enum ParamsIOFlag ioFlag);
   
   /**
    * @brief energyProbe: If nonblank, specifies the name of a ColumnEnergyProbe
    * that this probe contributes an energy term to.
    */
   virtual void ioParam_energyProbe(enum ParamsIOFlag ioFlag);
   
   /**
    * @brief coefficient: If energyProbe is set, the coefficient parameter specifies
    * that ColumnEnergyProbe multiplies the result of this probe's getValues() method
    * by coefficient when computing the error.
    * @details Note that coefficient does not affect the value returned by the getValue() or
    * getValues() method.
    */
   virtual void ioParam_coefficient(enum ParamsIOFlag ioFlag);
   /** @} */

   virtual int initOutputStream(const char * filename);

   /**
    * A pure virtual method for that should return true if the quantities being measured
    * by the probe have changed since the last time the quantities were calculated.
    * Typically, an implementation of needRecalc() will check the lastUpdateTime of
    * the object being probed, and return true if that value is greater than the
    * lastUpdateTime member variable.
    * needRecalc() is called by getValues(double) (and hence by getValue() and the other
    * flavors of getValues).
    * Note that there is a single needRecalc that applies to all getNumValues() quantities.
    */
   virtual bool needRecalc(double timevalue) = 0;

   /**
    * A pure virtual method that should return the simulation time for the values that calcValues()
    * would compute if it were called instead.  The reason that this time might be different from
    * the simuluation time at which referenceUpdate was called, is that calcValues might be called
    * either before or after the update of whatever object the probe is attached to.
    *
    * The getValues() method calls this function after calling calcValues(),
    * and stores the result in the lastUpdateTime member variable.  Typically, the implementation
    * of needRecalc() will return true if lastUpdateTime is less than the value returned by
    * referenceUpdateTime, and false otherwise.
    */
   virtual double referenceUpdateTime() const = 0;

   /**
    * A pure virtual method to calculate the values of the probe.  calcValues() can
    * assume that needRecalc() has been called and returned true.
    * It should write the computed values into the buffer of member variable 'probeValues'.
    */
   virtual int calcValues(double timevalue) = 0;

   /**
    * If needRecalc() returns true, getValues(double) updates the probeValues buffer
    * (by calling calcValues) and sets lastUpdateTime to the timevalue input argument.
    */
   int getValues(double timevalue);
   
   /**
    * Returns a pointer to the message parameter.
    */
   const char * getMessage() {return msgstring;}
   
   /**
    * The method called by BaseProbe::initialize() to set the message used by
    * the probe's outputState method.
    */
   virtual int initMessage(const char * msg);
   
   /**
    * Returns a pointer to the OutStream used by outputState.
    */
   OutStream * getOutputStream() {return outputStream;}

   /**
    * Returns a reference to the ostream that outputState writes to.
    */
   std::ostream& output() { return outputStream->outStream(); }
   
   /**
    * initNumValues is called by initialize.
    * BaseProbe::initNumValues sets numValues to the parent HyPerCol's getNBatch().
    * Derived classes can override initNumValues to initialize numValues to a different
    * value.
    */
   virtual int initNumValues();
   
   /**
    * Sets the numValues member variable (returned by getNumValues()) and reallocates
    * the probeValues member variable to hold numValues double-precision values.
    * If the reallocation fails, the probeValues buffer is left unchanged, errno is
    * set (by a realloc() call), and PV_FAILURE is returned.
    * Otherwise, PV_SUCCESS is returned.
    */
   int setNumValues(int n);
   
   /**
    * Returns a pointer to the buffer containing the probeValues.
    */
   double * getValuesBuffer() { return probeValues; }

   /**
    * Returns the time that calcValues was last called.
    * BaseProbe updates the last update time in getValues() and getValue(),
    * based on the result of needRecalc.
    */
   double getLastUpdateTime() { return lastUpdateTime; }

   /**
    * Returns the value of the textOutputFlag parameter
    */
   inline bool getTextOutputFlag() const { return textOutputFlag; }

   /**
    * Returns true if a probeOutputFile is being used.
    * Otherwise, returns false (indicating output is going to getOutputStream().
    */
   inline bool isWritingToFile() const { return writingToFile; }

private:
   int initialize_base();

// Member variables
protected:
   OutStream * outputStream;
   bool triggerFlag;
   char* triggerLayerName;
   HyPerLayer * triggerLayer;
   double triggerOffset;
   char * targetName;
   char * energyProbe; // the name of the ColumnEnergyProbe to attach to, if any.
   double coefficient;

private:
   char * msgparams; // the message parameter in the params
   char * msgstring; // the string that gets printed by outputState ("" if message is empty or null; message + ":" if nonempty
   char * probeOutputFilename;
   int numValues;
   double * probeValues;
   double lastUpdateTime; // The time of the last time calcValues was called.
   bool textOutputFlag;
   bool writingToFile; // true outputStream is a FileStream
};

}

#endif /* BASEPROBE_HPP_ */
