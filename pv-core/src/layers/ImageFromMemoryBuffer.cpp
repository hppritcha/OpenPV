/*
 * ImageFromMemoryBuffer.hpp
 *
 *  Created on: Oct 31, 2014
 *      Author: Pete Schultz
 *  Description of the class is in ImageFromMemoryBuffer.hpp
 */

#include "ImageFromMemoryBuffer.hpp"

namespace PV {

ImageFromMemoryBuffer::ImageFromMemoryBuffer(char const * name, HyPerCol * hc) {
   initialize_base();
   initialize(name, hc);
}

ImageFromMemoryBuffer::ImageFromMemoryBuffer() {
   initialize_base();
   // protected default constructor; initialize(name,hc) should be called by any derived class's initialization routine
}

int ImageFromMemoryBuffer::initialize_base() {
   buffer = NULL;
   bufferSize = -1;
   hasNewImageFlag = false;
   autoResizeFlag = false;
   resizeFactor = 1.0f;
   return PV_SUCCESS;
}

int ImageFromMemoryBuffer::initialize(char const * name, HyPerCol * hc) {
   return BaseInput::initialize(name, hc);
   if (useImageBCflag && autoResizeFlag) {
      if (parent->columnId()==0) {
         fprintf(stderr, "%s \"%s\" error: setting both useImageBCflag and autoResizeFlag has not yet been implemented.\n", getKeyword(), name);
      }
      MPI_Barrier(parent->icCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
}

int ImageFromMemoryBuffer::ioParamsFillGroup(enum ParamsIOFlag ioFlag) {
   int status = BaseInput::ioParamsFillGroup(ioFlag);
   ioParam_autoResizeFlag(ioFlag);
   return status;
}

void ImageFromMemoryBuffer::ioParam_autoResizeFlag(enum ParamsIOFlag ioFlag) {
   parent->ioParamValue(ioFlag, name, "autoResizeFlag", &autoResizeFlag, autoResizeFlag);
}

template <typename pixeltype>
int ImageFromMemoryBuffer::setMemoryBuffer(pixeltype const * externalBuffer, int height, int width, int numbands, int xstride, int ystride, int bandstride, pixeltype zeroval, pixeltype oneval) {
   if (height<=0 || width<=0 || numbands<=0) {
      if (parent->columnId()==0) {
         fprintf(stderr, "ImageFromMemoryBuffer::setMemoryBuffer error: height, width, numbands arguments must be positive.\n");
      }
      return PV_FAILURE;
   }

   int resizedWidth, resizedHeight;
   if (autoResizeFlag) {
      float xRatio = (float) getLayerLoc()->nxGlobal/(float) width;
      float yRatio = (float) getLayerLoc()->nyGlobal/(float) height;
      resizeFactor = xRatio < yRatio ? yRatio : xRatio;
      // resizeFactor * width should be >= getLayerLoc()->nx; resizeFactor * height should be >= getLayerLoc()->ny,
      // and one of these >= should be == (up to floating-point roundoff).
      resizedWidth = (int) nearbyintf(resizeFactor * width);
      resizedHeight = (int) nearbyintf(resizeFactor * height);
      assert(resizedWidth >= getLayerLoc()->nxGlobal);
      assert(resizedHeight >= getLayerLoc()->nyGlobal);
      assert(resizedWidth == getLayerLoc()->nxGlobal || resizedHeight == getLayerLoc()->nyGlobal);
   }
   else {
      resizedWidth = width;
      resizedHeight = height;
   }

   imageLoc.nbatch = 1;
   imageLoc.nx = resizedWidth;
   imageLoc.ny = resizedHeight;
   imageLoc.nf = numbands;
   imageLoc.nbatchGlobal = 1;
   imageLoc.nxGlobal = resizedWidth;
   imageLoc.nyGlobal = resizedHeight;
   imageLoc.kb0 = 0;
   imageLoc.kx0 = 0;
   imageLoc.ky0 = 0;
   memset(&imageLoc.halo, 0, sizeof(PVHalo));

   if (parent->columnId()==0) {
      int newBufferSize = resizedWidth * resizedHeight * numbands;
      if (newBufferSize != bufferSize) {
         free(buffer);
         bufferSize = newBufferSize;
         buffer = (pvadata_t *) malloc((size_t) bufferSize * sizeof(pvadata_t));
         if (buffer==NULL) {
            fprintf(stderr, "%s \"%s\": unable to allocate buffer for %d values of %zu chars each: %s\n",
                  getKeyword(), name, bufferSize, sizeof(pvadata_t), strerror(errno));
            exit(EXIT_FAILURE);
         }
      }
      if (autoResizeFlag) {
         int const nxExtern = imageLoc.nxGlobal;
         int const nyExtern = imageLoc.nyGlobal;
         bicubicinterp(externalBuffer, height, width, numbands, xstride, ystride, bandstride, resizedHeight, resizedWidth, zeroval, oneval);
      }
      else {
         #ifdef PV_USE_OPENMP_THREADS
         #pragma omp parallel for
         #endif // PV_USE_OPENMP_THREADS
         for (int k=0; k<bufferSize; k++) {
            int x=kxPos(k,width,height,numbands);
            int y=kyPos(k,width,height,numbands);
            int f=featureIndex(k,width,height,numbands);
            int externalIndex = f*bandstride + x*xstride + y*ystride;
            pixeltype q = externalBuffer[externalIndex];
            buffer[k] = pixelTypeConvert(q, zeroval, oneval);
         }
      }

      // Code duplication from Image::readImage
      // if normalizeLuminanceFlag == true and normalizeStdDev == true, then force average luminance to be 0, std. dev.=1
      // if normalizeLuminanceFlag == true and normalizeStdDev == false, then force min=0, max=1.
      bool normalize_standard_dev = normalizeStdDev;
      if(normalizeLuminanceFlag){
         if (normalize_standard_dev){
            double image_sum = 0.0f;
            double image_sum2 = 0.0f;
            for (int k=0; k<bufferSize; k++) {
               image_sum += buffer[k];
               image_sum2 += buffer[k]*buffer[k];
            }
            double image_ave = image_sum / bufferSize;
            double image_ave2 = image_sum2 / bufferSize;
            // set mean to zero
            #ifdef PV_USE_OPENMP
            #pragma omp parallel for
            #endif // PV_USE_OPENMP
            for (int k=0; k<bufferSize; k++) {
               buffer[k] -= image_ave;
            }
            // set std dev to 1
            double image_std = sqrt(image_ave2 - image_ave*image_ave);
            if(image_std == 0){
               #ifdef PV_USE_OPENMP
               #pragma omp parallel for
               #endif // PV_USE_OPENMP
               for (int k=0; k<bufferSize; k++) {
                  buffer[k] = .5;
               }
            }
            else{
               #ifdef PV_USE_OPENMP
               #pragma omp parallel for
               #endif // PV_USE_OPENMP
               for (int k=0; k<bufferSize; k++) {
                  buffer[k] /= image_std;
               }
            }
         }
         else{
            float image_max = -FLT_MAX;
            float image_min = FLT_MAX;
            for (int k=0; k<bufferSize; k++) {
               image_max = buffer[k] > image_max ? buffer[k] : image_max;
               image_min = buffer[k] < image_min ? buffer[k] : image_min;
            }
            if (image_max > image_min){
               float image_stretch = 1.0f / (image_max - image_min);
               #ifdef PV_USE_OPENMP
               #pragma omp parallel for
               #endif // PV_USE_OPENMP
               for (int k=0; k<bufferSize; k++) {
                  buffer[k] -= image_min;
                  buffer[k] *= image_stretch;
               }
            }
            else{ // image_max == image_min, set to gray
               //float image_shift = 0.5f - image_ave;
               #ifdef PV_USE_OPENMP
               #pragma omp parallel for
               #endif // PV_USE_OPENMP
               for (int k=0; k<bufferSize; k++) {
                  buffer[k] = 0.5f; //image_shift;
               }
            }
         }
      } // normalizeLuminanceFlag

      if( inverseFlag ) {
         #ifdef PV_USE_OPENMP
         #pragma omp parallel for
         #endif // PV_USE_OPENMP
         for (int k=0; k<bufferSize; k++) {
            buffer[k] = 1 - buffer[k];
         }
      }
      // Code duplication from Image::readImage

   }
   
   hasNewImageFlag = true;

   return PV_SUCCESS;
}
template int ImageFromMemoryBuffer::setMemoryBuffer<uint8_t>(uint8_t const * buffer, int height, int width, int numbands, int xstride, int ystride, int bandstride, uint8_t zeroval, uint8_t oneval);

template <typename pixeltype>
int ImageFromMemoryBuffer::setMemoryBuffer(pixeltype const * externalBuffer, int height, int width, int numbands, int xstride, int ystride, int bandstride, pixeltype zeroval, pixeltype oneval, int offsetX, int offsetY, char const * offsetAnchor) {
   offsets[0] = offsetX;
   offsets[1] = offsetY;
   free(this->offsetAnchor);
   this->offsetAnchor = strdup(offsetAnchor);
   if (checkValidAnchorString()!=PV_SUCCESS) {
      if (parent->columnId()==0) {
         fprintf(stderr, "%s \"%s\" error: setMemoryBuffer called with invalid anchor string \"%s\"",
               getKeyword(), name, offsetAnchor);
      }
      MPI_Barrier(parent->icCommunicator()->communicator());
      exit(EXIT_FAILURE);
   }
   return setMemoryBuffer(externalBuffer, height, width, numbands, xstride, ystride, bandstride, zeroval, oneval);
}
template int ImageFromMemoryBuffer::setMemoryBuffer<uint8_t>(uint8_t const * buffer, int height, int width, int numbands, int xstride, int ystride, int bandstride, uint8_t zeroval, uint8_t oneval, int offsetX, int offsetY, char const * offsetAnchor);

template <typename pixeltype>
pvadata_t ImageFromMemoryBuffer::pixelTypeConvert(pixeltype q, pixeltype zeroval, pixeltype oneval) {
   return ((pvadata_t) (q-zeroval))/((pvadata_t) (oneval-zeroval));
}

template <typename pixeltype> int ImageFromMemoryBuffer::bicubicinterp(pixeltype const * bufferIn, int heightIn, int widthIn, int numBands, int xStrideIn, int yStrideIn, int bandStrideIn, int heightOut, int widthOut, pixeltype zeroval, pixeltype oneval) {
   /* Interpolation using bicubic convolution with a=-1 (following Octave image toolbox's imremap function -- change this?). */
   float xinterp[widthOut];
   int xinteger[widthOut];
   float xfrac[widthOut];
   float dx = (float) (widthIn-1)/(float) (widthOut-1);
   for (int kx=0; kx<widthOut; kx++) {
      float x = dx * (float) kx;
      xinterp[kx] = x;
      float xfloor = floorf(x);
      xinteger[kx] = (int) xfloor;
      xfrac[kx] = x-xfloor;
   }

   float yinterp[heightOut];
   int yinteger[heightOut];
   float yfrac[heightOut];
   float dy = (float) (heightIn-1)/(float) (heightOut-1);
   for (int ky=0; ky<heightOut; ky++) {
      float y = dy * (float) ky;
      yinterp[ky] = y;
      float yfloor = floorf(y);
      yinteger[ky] = (int) yfloor;
      yfrac[ky] = y-yfloor;
   }

   memset(buffer, 0, sizeof(*buffer)*bufferSize);
   for (int xOff = 2; xOff > -2; xOff--) {
      for (int yOff = 2; yOff > -2; yOff--) {
         for (int ky=0; ky<heightOut; ky++) {
            float ycoeff = bicubic(yfrac[ky]-(float) yOff);
            int yfetch = yinteger[ky]+yOff;
            if (yfetch < 0) yfetch = -yfetch;
            if (yfetch >= heightIn) yfetch = heightIn - (yfetch - heightIn) - 1;
            for (int kx=0; kx<widthOut; kx++) {
               float xcoeff = bicubic(xfrac[kx]-(float) xOff);
               int xfetch = xinteger[kx]+xOff;
               if (xfetch < 0) xfetch = -xfetch;
               if (xfetch >= widthIn) xfetch = widthIn - (xfetch - widthIn) - 1;
               assert(xfetch >= 0 && xfetch < widthIn && yfetch >= 0 && yfetch < heightIn);
               for (int f=0; f<numBands; f++) {
                  int fetchIdx = yfetch * yStrideIn + xfetch * xStrideIn + f * bandStrideIn;
                  pvadata_t p = pixelTypeConvert(bufferIn[fetchIdx], zeroval, oneval);
                  int outputIdx = kIndex(kx, ky, f, widthOut, heightOut, numBands);
                  buffer[outputIdx] += xcoeff * ycoeff * p;
               }
            }
         }
      }
   }
   return PV_SUCCESS;
}

int ImageFromMemoryBuffer::initializeActivity(double time, double dt) {
   return getFrame(time, dt);
}

int ImageFromMemoryBuffer::updateState(double time, double dt) {
   return getFrame(time, dt);
}

//Image readImage reads the same thing to every batch
//This call is here since this is the entry point called from allocate
//Movie overwrites this function to define how it wants to load into batches
int ImageFromMemoryBuffer::retrieveData(double timef, double dt)
{
   int status = PV_SUCCESS;
   status = copyBuffer();
   return status;
}

int ImageFromMemoryBuffer::copyBuffer() {
   int status = PV_SUCCESS;
   if (!hasNewImageFlag) { return status; }
   Communicator * icComm = parent->icCommunicator();
   if (parent->columnId()==0) {
      if (buffer == NULL) {
         fprintf(stderr, "%s \"%s\" error: copyBuffer called without having called setMemoryBuffer.\n",
               getKeyword(), name);
         exit(PV_FAILURE); // return PV_FAILURE;
      }
      for (int rank=1; rank<icComm->commSize(); rank++) {
         status = moveBufferToData(rank);
         assert(status == PV_SUCCESS);
         MPI_Send(data, getNumExtended(), MPI_FLOAT, rank, 31, icComm->communicator());
      }
      status = moveBufferToData(0);
      assert(status == PV_SUCCESS);
   }
   else {
      MPI_Recv(data, getNumExtended(), MPI_FLOAT, 0, 31, icComm->communicator(), MPI_STATUS_IGNORE);
   }
   hasNewImageFlag = false;
   return status;
}

int ImageFromMemoryBuffer::moveBufferToData(int rank) {
   assert(parent->columnId()==0);
   assert(buffer != NULL);
   
   // The part of the buffer with x in [processcolumnnumber * nx, processcolumnnumber * (nx + 1) - 1]
   // and y in [processcolumnnumber * nx, processcolumnnumber * (nx + 1) - 1]
   // gets copied to the restricted space of the given rank.
   // If useImageBCflag is set, we also have to copy as much of the surrounding region as
   // will fit in both the halo and the image.
   // Are we also supposed to mirror any gap between the edge of the image and the edge of the halo?
   // The code doesn't do that at this point.
   // The code below assumes that all processes have the same getLayerLoc().
   Communicator * icComm = parent->icCommunicator();
   int column = columnFromRank(rank, icComm->numCommRows(), icComm->numCommColumns());
   int startxbuffer = getOffsetX(this->offsetAnchor, this->offsets[0]) + column * getLayerLoc()->nx;
   int startxdata = getLayerLoc()->halo.lt;
   int row = rowFromRank(rank, icComm->numCommRows(), icComm->numCommColumns());
   int startybuffer = getOffsetY(this->offsetAnchor, this->offsets[1]) + row * getLayerLoc()->ny;
   int startydata = getLayerLoc()->halo.up;
   int xsize = getLayerLoc()->nx;
   int ysize = getLayerLoc()->ny;
   int fsize = getLayerLoc()->nf;
   PVHalo const * halo = &getLayerLoc()->halo;
   if (useImageBCflag) {

      int moveleft = startxbuffer;
      if (halo->lt < moveleft) {
         moveleft = halo->lt;
      }
      if (moveleft > 0) {
         startxbuffer -= moveleft;
         startxdata -= moveleft;
         xsize += moveleft;
      }

      int moveright = imageLoc.nx - (startxbuffer + xsize);
      if (halo->rt < moveright) {
         moveright = halo->rt;
      }
      if (moveright > 0) {
         xsize += moveright;
      }

      int moveup = startybuffer;
      if (halo->up < moveup) {
         moveup = halo->up;
      }
      if (moveup > 0) {
         startybuffer -= moveup;
         startydata -= moveup;
         ysize += moveup;
      }

      int movedown = imageLoc.ny - (startybuffer + ysize);
      if (halo->dn < movedown) {
         movedown = halo->dn;
      }
      if (movedown > 0) {
         ysize += movedown;
      }   
   }
   assert(startxbuffer >= 0 && startxbuffer + xsize <= imageLoc.nxGlobal);
   assert(startybuffer >= 0 && startybuffer + ysize <= imageLoc.nyGlobal);
   if (fsize != 1 && imageLoc.nf != 1 && fsize != imageLoc.nf) {
      fprintf(stderr, "%s \"%s\": If nf and the number of bands in the image are both greater than 1, they must be equal.\n", getKeyword(), name);
      exit(EXIT_FAILURE);
   }
   if (fsize == 1 && imageLoc.nf > 1) {
      // layer has one feature; convert memory buffer to grayscale
      for (int y=0; y<ysize; y++) {
         for (int x=0; x<xsize; x++) {
            int indexdata = kIndex(startxdata+x,startydata+y,0,getLayerLoc()->nx+halo->lt+halo->rt,getLayerLoc()->ny+halo->dn+halo->up,1);
            int indexbuffer = kIndex(startxdata+x,startydata+y,0,imageLoc.nx,imageLoc.ny,imageLoc.nf);
            pvdata_t val = (pvadata_t) 0;
            for (int f=0; f<imageLoc.nf; f++) {
               val += buffer[indexbuffer+f];
            }
            data[indexdata] = val/(pvadata_t) imageLoc.nf;
         }
      }
   }
   else if (fsize > 1 && imageLoc.nf == 1) {
      // image is grayscale; replicate over all color bands of layer
      for (int y=0; y<ysize; y++) {
         for (int x=0; x<xsize; x++) {
            for (int f=0; f<fsize; f++) {
               int indexdata = kIndex(startxdata+x,startydata+y,f,getLayerLoc()->nx+halo->lt+halo->rt,getLayerLoc()->ny+halo->dn+halo->up,getLayerLoc()->nf);
               int indexbuffer = kIndex(startxdata+x,startydata+y,0,imageLoc.nx,imageLoc.ny,imageLoc.nf);
               data[indexdata] = buffer[indexbuffer];
            }
         }
      }
   }
   else {
      assert(fsize == imageLoc.nf); // layer and memory buffer have the same number of features
      for (int y=0; y<ysize; y++) {
         int linestartdata = kIndex(startxdata,startydata+y,0,getLayerLoc()->nx+halo->lt+halo->rt,getLayerLoc()->ny+halo->dn+halo->up,fsize);
         int linestartbuffer = kIndex(startxbuffer,startybuffer+y,0,imageLoc.nx,imageLoc.ny,fsize);
         memcpy(&data[linestartdata], &buffer[linestartbuffer], sizeof(pvadata_t) * xsize * fsize);
      }
   }
   return PV_SUCCESS;
}

double ImageFromMemoryBuffer::getDeltaUpdateTime(){
   return parent->getStopTime() - parent->getStartTime();
}

int ImageFromMemoryBuffer::outputState(double time, bool last) {
   return HyPerLayer::outputState(time, last);
}


ImageFromMemoryBuffer::~ImageFromMemoryBuffer() {
   free(buffer);
}

}  // namespace PV
