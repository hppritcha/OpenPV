/*
 * LogLatWTAProbe.hpp
 *
 * A derived class of LayerFunctionProbe that uses LogLatWTAFunction
 *
 *  Created on: Apr 26, 2011
 *      Author: peteschultz
 */

#ifndef LOGLATWTAPROBE_HPP_
#define LOGLATWTAPROBE_HPP_

#include "LayerFunctionProbe.hpp"
#include "LogLatWTAFunction.hpp"

namespace PV {

class LogLatWTAProbe : public LayerFunctionProbe {
public:
   LogLatWTAProbe(const char * probeName, HyPerCol * hc);
   virtual ~LogLatWTAProbe();

protected:
   LogLatWTAProbe();
   int initLogLatWTAProbe(const char * probeName, HyPerCol * hc);
   void initFunction();
   virtual int writeState(double timed, HyPerLayer * l, pvdata_t value);

private:
   int initLogLatWTAProbe_base() { return PV_SUCCESS; }
};

}  // end namespace PV



#endif /* LOGLATWTAPROBE_HPP_ */
