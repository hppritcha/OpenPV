/*
 * Movie.cpp
 *
 *  Created on: Sep 25, 2009
 *      Author: travel
 */

#include "Movie.hpp"
#include "../io/imageio.hpp"
#include "../utils/pv_random.h"
#include "../include/default_params.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace PV {

Movie::Movie(const char * name, HyPerCol * hc, const char * fileOfFileNames)
     : Image(name, hc)
{
    initializeMovie(name, hc, fileOfFileNames, DISPLAY_PERIOD);
}

Movie::Movie(const char * name, HyPerCol * hc, const char * fileOfFileNames, float displayPeriod)
     : Image(name, hc)
{
   initializeMovie(name, hc, fileOfFileNames, displayPeriod);
}


int Movie::initializeMovie(const char * name, HyPerCol * hc, const char * fileOfFileNames, float displayPeriod) {

   PVLayerLoc * loc = &clayer->loc;


   fp = fopen(fileOfFileNames, "r");
   assert(fp != NULL);

   filename = strdup(getNextFileName());
   assert(filename != NULL);

   // get size info from image so that data buffer can be allocated
   int status = getImageInfo(filename, parent->icCommunicator(), &imageLoc);
   assert(status == 0);

   // create mpi_datatypes for border transfer
   mpi_datatypes = Communicator::newDatatypes(loc);

//   int N = imageLoc.nx * imageLoc.ny * imageLoc.nBands;
//   imageData = new float [N];
//   for (int i = 0; i < N; ++i) {
//      imageData[i] = 0;
//   }
   imageData = NULL;

   // need all image bands until converted to gray scale
   loc->nBands = imageLoc.nBands;

#ifdef OBSOLETE
   initialize_data(loc);

//   N = loc.nx * loc.ny * loc.nBands;
//   imageData = new float [N];
//   for (int i = 0; i < N; ++i) {
//      imageData[i] = 0;
//   }
//
#endif

   //
   PVParams * params = hc->parameters();
   this->displayPeriod = params->value(name,"displayPeriod", displayPeriod);
   this->nextDisplayTime = hc->simulationTime() + this->displayPeriod;
   stepSize          = (int) params->value(name, "stepSize", 0);
   recurrenceProb    = params->value(name,"recurrenceProb", 1.0);
   persistenceProb   = params->value(name,"persistenceProb", 1.0);
   biasChangeTime    = (int) params->value(name,"biasChangeTime", 1000);
   writePosition     = (int) params->value(name,"writePosition", 1);
   randomMovie       = (int) params->value(name,"randomMovie",0);
   randomMovieProb   = params->value(name,"randomMovieProb", 0.05);  // 100 Hz
   offsetX           = (int) params->value(name,"offsetX", 0);
   offsetY           = (int) params->value(name,"offsetY", 0);

   resetPositionInBounds();  // ensure that offsets keep loc within image bounds
   
   biasX   = offsetX;
   biasY   = offsetY;

   if(writePosition){
      fp_pos = fopen("input/image-pos.txt","a");
      assert(fp_pos != NULL);
      fprintf(fp_pos,"%f %s: \n%d %d\t\t%f %d %d\n",hc->simulationTime(),filename,biasX,biasY,
            hc->simulationTime(),offsetX,offsetY);
   }

   if(randomMovie){
      unsigned long seed = (unsigned long) time ((time_t *) NULL);
      pv_srandom(seed); // initialize seed
      randomFrame();
   }else{
      read(filename, offsetX, offsetY);
   }

   // for now convert images to grayscale
   if (loc->nBands > 1) {
      toGrayScale();
   }

   // exchange border information
   exchange();
}

Movie::~Movie()
{
   if (imageData != NULL) {
      delete imageData;
      imageData = NULL;
   }
   if (fp != NULL && fp != stdout) {
      fclose(fp);
   }

   if(writePosition){
      if (fp_pos != NULL && fp_pos != stdout) {
            fclose(fp_pos);
         }
   }
}

pvdata_t * Movie::getImageBuffer()
{
//   return imageData;
   return data;
}

PVLayerLoc Movie::getImageLoc()
{
//   return imageLoc;
   return clayer->loc;
}

int Movie::updateState(float time, float dt)
{
  updateImage(time, dt);
  return 0;
}

/**
 * - update the image buffers
 * - If the time is a multiple of biasChangetime then the position of the bias (biasX, biasY) changes.
 * - with probability persistenceProb the offset position (offsetX, offsetY) remains uncganhed.
 * - otherwise, with probability (1-persistenceProb) the offset position performs a random walk
 * around the bias position (biasX, biasY).
 * - return true if buffers have changed
 */
bool Movie::updateImage(float time, float dt)
{
   PVLayerLoc * loc = &clayer->loc;

   if(randomMovie){
      randomFrame();
   } else {
      if (time >= nextDisplayTime) {
         if (filename != NULL) free(filename);
         filename = strdup(getNextFileName());
         assert(filename != NULL);
         nextDisplayTime += displayPeriod;

         if(writePosition){
            fprintf(fp_pos,"%f %s: \n",time,filename);
         }
      }

      // need all image bands until converted to gray scale
      loc->nBands = imageLoc.nBands;

      // move bias
      if( time > 0 && !(((int)time) % biasChangeTime) ){
         calcBias(stepSize, imageLoc.nx - loc->nx - stepSize);
      }

      // move offset
      double p = pv_random_prob();
      if (p > persistenceProb){
         calcBiasedOffset(stepSize, imageLoc.nx - loc->nx - stepSize);
         if(writePosition){
            fprintf(fp_pos,"%d %d ",biasX,biasY);
         }
      }


      // ensure that offsets keep loc within image bounds
      resetPositionInBounds();

      if(writePosition){
         fprintf(fp_pos,"\t\t%f %d %d\n",time,offsetX,offsetY);
      }
      read(filename, offsetX, offsetY);

      // for now convert images to grayscale
      if (loc->nBands > 1) {
         toGrayScale();
      }
   } // randomMovie

   // exchange border information
   exchange();

   lastUpdateTime = time;

   return true;
}

/**
 * When we play a random frame - in order to perform a reverse correlation analysis -
 * we call writeActivitySparse(time) in order to write the "activity" in the image.
 *
 */
int Movie::outputState(float time, bool last)
{
   if (writeImages) {
      char basicFilename[PV_PATH_MAX + 1];
      snprintf(basicFilename, PV_PATH_MAX, "./output/Movie_%f.tif", time);
      write(basicFilename);
   }

   if (randomMovie != 0) {
      int status = 0;
      status = writeActivitySparse(time);
   }

   return 0;
}

int Movie::copyReducedImagePortion()
{
   const PVLayerLoc * loc = getLayerLoc();

   const int nx = loc->nx;
   const int ny = loc->ny;

   const int nx0 = imageLoc.nx;
   const int ny0 = imageLoc.ny;

   assert(nx0 <= nx);
   assert(ny0 <= ny);

   const int i0 = nx/2 - nx0/2;
   const int j0 = ny/2 - ny0/2;

   int ii = 0;
   for (int j = j0; j < j0+ny0; j++) {
      for (int i = i0; i < i0+nx0; i++) {
         imageData[ii++] = data[i+nx*j];
      }
   }

   return 0;
}

/**
 * This creates a random image patch (frame) that is used to perform a reverse correlation analysis
 * as the input signal propagates up the visual system's hierarchy.
 * NOTE: Check Image::toGrayScale() method which was the inspiration for this routine
 */
int Movie::randomFrame()
{
   const PVLayerLoc * loc = getLayerLoc();

   const int nx = loc->nx;
   const int ny = loc->ny;
   const int numBands = loc->nBands;  // same with nf
   const int marginWidth = loc->nPad;


   int numActive = 0;
   for (int kex = 0; kex < clayer->numExtended; kex++) {
      const int k = kIndexRestricted(kex, nx, ny, numBands, marginWidth);
      data[kex] = (pv_random_prob() < randomMovieProb) ? 1: 0;
      if (k > 0 && data[kex] > 0.0) {
         clayer->activeIndices[numActive++] = k;
      }
   }
   clayer->numActive = numActive;

   return 0;
}

const char * Movie::getNextFileName()
{
   int c;
   size_t len = PV_PATH_MAX;
   char * path;

   // if at end of file (EOF), rewind

   if ((c = fgetc(fp)) == EOF) {
      rewind(fp);
   }
   else {
      ungetc(c, fp);
   }

   path = fgets(this->inputfile, len, fp);

   if (path != NULL) {
      path[PV_PATH_MAX-1] = '\0';
      len = strlen(path);
      if (len > 1) {
         if (path[len-1] == '\n') {
             path[len-1] = '\0';
         }
      }
   }

   return path;
}


/**
 * The bias position (biasX, biasY) changes here.
 * It can perform a random walk of step 1 or it can perform a random jump up to a maximum length
 * equal to step.
 */
void Movie::calcBias(int step, int sizeLength)
{
   const float dp = 1.0 / step;
   double p;
   const int random_walk = 0;
   const int random_jump = 1;


   if (random_walk) {
      p = pv_random_prob();
      if (p < 0.5) {
         biasX += step;
      } else {
         biasX -= step;
      }
      p = pv_random_prob();
      if (p < 0.5) {
         biasY += step;
      } else {
         biasY -= step;
      }
   } else if (random_jump) {
      p = pv_random_prob();
      for (int i = 0; i < step; i++) {
         if ((i * dp < p) && (p < (i + 1) * dp)) {
            biasX = (pv_random_prob() < 0.5) ? biasX - i : biasX + i;
         }
      }
      p = pv_random_prob();
      for (int i = 0; i < step; i++) {
         if ((i * dp < p) && (p < (i + 1) * dp)) {
            biasY = (pv_random_prob() < 0.5) ? biasY - i : biasY + i;
         }
      }
   }

   biasX = (biasX < 0) ? -biasX : biasX;
   biasX = (biasX > sizeLength) ? sizeLength - (biasX-sizeLength) : biasX;

   biasY = (biasY < 0) ? -biasY : biasY;
   biasY = (biasY > sizeLength) ? sizeLength - (biasY-sizeLength) : biasY;

   return;
}

/**
 * Return an offset that moves randomly around position (biasX, biasY)
 * With probability recurenceProb the offset returns to its bias position
 * (biasX,biasY). Otherwise, with probability (1-recurrenceProb) performa a
 * random jump of maximum length equal to step.
 */
void Movie::calcBiasedOffset(int step, int sizeLength)
{
   const float dp = 1.0 / step;
   double p = pv_random_prob();

   if (p > recurrenceProb){
      p = pv_random_prob();
      for (int i = 0; i < step; i++) {
         if ((i * dp < p) && (p < (i + 1) * dp)) {
            offsetX = (pv_random_prob() < 0.5) ? offsetX - i : offsetX + i;
         }
      }
      p = pv_random_prob();
      for (int i = 0; i < step; i++) {
         if ((i * dp < p) && (p < (i + 1) * dp)) {
            offsetY = (pv_random_prob() < 0.5) ? offsetY - i : offsetY + i;
         }
      }
      offsetX = (offsetX < 0) ? -offsetX : offsetX;
      offsetX = (offsetX > sizeLength) ? sizeLength - (offsetX-sizeLength) : offsetX;

      offsetY = (offsetY < 0) ? -offsetY : offsetY;
      offsetY = (offsetY > sizeLength) ? sizeLength - (offsetY-sizeLength) : offsetY;
   } else {
      offsetX = biasX;
      offsetY = biasY;
   }

   return;
}


/**
 * Return an integer between 0 and (step-1)
 */
int Movie::calcPosition(int pos, int step, int sizeLength)
{
   const float dp = 1.0 / step;
   const double p = pv_random_prob();
   const int random_walk = 0;
   const int move_forward = 0;
   const int move_backward = 0;
   const int random_jump = 1;

   if (random_walk) {
      if (p < 0.5) {
         pos += step;
      } else {
         pos -= step;
      }
   } else if (move_forward) {
      pos += step;
   } else if (move_backward) {
      pos -= step;
   } else if (random_jump) {
      for (int i = 0; i < step; i++) {
         if ((i * dp < p) && (p < (i + 1) * dp)) {
            pos = (pv_random_prob() < 0.5) ? pos - i : pos + i;
         }
      }
   }

   pos = (pos < 0) ? -pos : pos;
   pos = (pos > sizeLength) ? sizeLength - (pos-sizeLength) : pos;

   return pos;
}


int Movie::resetPositionInBounds()
{
   PVLayerLoc * loc = &clayer->loc;

   // apply circular boundary conditions
   //
   if (offsetX < 0) offsetX += imageLoc.nx;
   if (offsetY < 0) offsetY += imageLoc.ny;
   offsetX = (imageLoc.nx < offsetX + loc->nx) ? imageLoc.nx - offsetX : offsetX;
   offsetY = (imageLoc.ny < offsetY + loc->ny) ? imageLoc.ny - offsetY : offsetY;

   // could still be out of bounds
   //
   offsetX = (offsetX < 0 || imageLoc.nx < offsetX + loc->nx) ? offsetX = 0 : offsetX;
   offsetY = (offsetY < 0 || imageLoc.ny < offsetY + loc->ny) ? offsetY = 0 : offsetY;

   return 0;
}

}
