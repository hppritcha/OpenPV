/*
 * RandomConn.hpp
 *
 *  Created on: Apr 27, 2009
 *      Author: rasmussn
 */

#ifndef RANDOMCONN_HPP_
#define RANDOMCONN_HPP_

#include "HyPerConn.hpp"

namespace PV {

enum RandDistType {
   UNDEFINED = 0,
   UNIFORM = 1,
   GAUSSIAN = 2
};

class RandomConn: public PV::HyPerConn {
public:
   RandomConn(const char * name, HyPerCol * hc, HyPerLayer * pre,
              HyPerLayer * post, ChannelType channel);

   RandomConn(const char * name, HyPerCol * hc, HyPerLayer * pre,
              HyPerLayer * post, ChannelType channel, RandDistType distrib);

   virtual int initializeRandomWeights(unsigned long seed);
   int initializeUniformWeights(unsigned long seed);
   int uniformWeights(PVPatch * wp, float wMin, float wMax, unsigned long seed);
   int initializeGaussianWeights(unsigned long seed);
   int gaussianWeights(PVPatch *wp, float mean, float stdev, unsigned long seed);
   int gaussianWeightsMA(PVPatch *wp, float mean, float stdev, long *);
   float randgauss(float mean, float stdev);
   float randgaussMA(float mean, float stdev, long *);
private:
   float          wMin;
   RandDistType   randDistType; // the type of distribution
   float          wGaussMean;   // mean of the Gaussian distribution
   float          wGaussStdev;  // std deviation of the Gaussian distribution
   long           idum;

   float ran1(long *);
};

}

#endif /* RANDOMCONN_HPP_ */
