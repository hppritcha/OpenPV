/*
 * CudaBuffer.hpp
 *
 *  Created on: Aug 6, 2014
 *      Author: Sheng Lundquist
 */

#ifndef CUDABUFFER_HPP_
#define CUDABUFFER_HPP_

#include "arch/cuda/CudaDevice.hpp"
#include <cuda_runtime_api.h>

////////////////////////////////////////////////////////////////////////////////

namespace PVCuda {

class CudaDevice;

/**
 * A class to handle device memory allocations and transfers
 */
class CudaBuffer {

public:

   /**
    * Constructor to create a buffer of size inSize on a given stream
    * @param inSize The size of the buffer to create on the device
    * @param stream The cuda stream any transfer commands should go on
    */
   CudaBuffer(size_t inSize, CudaDevice * inDevice);
   virtual ~CudaBuffer();
   
   /**
    * A function to copy host memory to device memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param h_ptr The pointer to the host address to copy to the device
    * #return Returns PV_Success if successful
    */
   virtual int copyToDevice(const void * h_ptr);

   /**
    * A function to copy device memory to host memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param h_ptr The pointer to the host address to copy from the device
    * @param in_size The size of the data to copy. Defaults to the size of the buffer.
    * #return Returns PV_Success if successful
    */
   virtual int copyFromDevice(void* h_ptr);
   virtual int copyFromDevice(void* h_ptr, size_t in_size);

   /**
    * A getter function to return the device pointer allocated
    * #return Returns the device pointer
    */
   virtual void* getPointer(){return d_ptr;}

   /**
    * A getter function to return the size of the device memory
    * #return Returns the size of the device memory
    */
   size_t getSize(){return size;}

   void permuteWeightsPVToCudnn(void *d_inPtr, int numArbors, int numKernels, int nxp, int nyp, int nfp);

protected:
   void * d_ptr;                       // pointer to buffer on host
   size_t size;
   CudaDevice * device;

private:
   /**
    * A function to copy host memory to device memory. Note that the host and device memory must have the same size, otherwise undefined behavior
    * @param h_ptr The pointer to the host address to copy to the device
    * @param in_size The size of the data to copy. Defaults to the size of the buffer.
    * #return Returns PV_Success if successful
    */
   virtual int copyToDevice(const void * h_ptr, size_t in_size);
   void callCudaPermuteWeightsPVToCudnn(int gridSize, int blockSize, void* d_inPtr, int numArbors, int outFeatures, int ny, int nx, int inFeatures);
}; // end class CudaBuffer

} // end namespace PVCuda

#endif
