/*
 * CudaDevice.cu
 *
 *  Created on: Aug 5, 2014
 *      Author: Sheng Lundquist
 */

#include "cuda_util.hpp"
#include "CudaDevice.hpp"

#ifdef PV_USE_CUDNN
#include <cudnn.h>
#endif

namespace PVCuda{

CudaDevice::CudaDevice(int device)
{
   this->device_id = device;
   this->handle = NULL;
   initialize(device_id);
}

CudaDevice::~CudaDevice()
{
   handleError(cudaStreamDestroy(stream), "Cuda Device Destructor");
   //TODO set handleError to take care of this

#ifdef PV_USE_CUDNN
   if(handle){
      cudnnDestroy((cudnnHandle_t)handle);
   }
#endif
}

int CudaDevice::getNumDevices(){
   int returnVal;
   handleError(cudaGetDeviceCount(&returnVal), "Static getting device count");
   return returnVal;
}

int CudaDevice::initialize(int device)
{
   int status = 0;

#ifdef PV_USE_CUDA
   handleError(cudaThreadExit(), "Thread exiting in initialize");

   handleError(cudaGetDeviceCount(&num_devices), "Getting device count");
   handleError(cudaSetDevice(device), "Setting device");

   handleError(cudaStreamCreate(&stream), "Creating stream");

   handleError(cudaGetDeviceProperties(&device_props, device), "Getting device properties");

   status = 0;
#endif // PV_USE_OPENCL
   
#ifdef PV_USE_CUDNN
   //Testing cudnn here
   cudnnHandle_t tmpHandle;
   cudnnStatus_t cudnnStatus = cudnnCreate(&tmpHandle); 
   if(cudnnStatus != CUDNN_STATUS_SUCCESS){
      switch(cudnnStatus){
         case CUDNN_STATUS_NOT_INITIALIZED:
            printf("cuDNN Runtime API initialization failed\n");
            break;
         case CUDNN_STATUS_ALLOC_FAILED:
            printf("cuDNN resources could not be allocated\n");
            break;
         default:
            printf("cuDNN unknown error\n");
      }
      exit(-1);
   }
   cudnnStatus = cudnnSetStream(tmpHandle, stream);
   if(cudnnStatus != CUDNN_STATUS_SUCCESS){
      printf("CUDNN Stream set error\n");
      exit(-1);
   }

   this->handle = (void*) tmpHandle;
#endif

   return status;
}

void CudaDevice::syncDevice(){
   handleError(cudaDeviceSynchronize(), "Synchronizing device");
}

int CudaDevice::query_device_info()
{
   // query and print information about the devices found
   //
   printf("\n");
   printf("Number of Cuda devices found: %d\n", num_devices);
   printf("\n");

   for (unsigned int i = 0; i < num_devices; i++) {
      query_device(i);
   }
   return 0;
}

CudaBuffer* CudaDevice::createBuffer(size_t size){
   return(new CudaBuffer(size, stream));
}

void CudaDevice::query_device(int id)
{
   struct cudaDeviceProp props;
   //Use own props if current device
   if(id == device_id){
      props = device_props;
   }
   //Otherwise, generate props
   else{
      handleError(cudaGetDeviceProperties(&props, id), "Getting device properties");
   }
   printf("device: %d\n", id);
   printf("CUDA Device # %d == %s\n", id, props.name);

   printf("with %d units/cores", props.multiProcessorCount);

   printf(" at %f MHz\n", (float)props.clockRate * .001);

   printf("\tMaximum threads group size == %d\n", props.maxThreadsPerBlock);
   
   printf("\tMaximum grid sizes == (");
   for (unsigned int i = 0; i < 3; i++) printf(" %d", props.maxGridSize[i]);
   printf(" )\n");

   printf("\tMaximum threads sizes == (");
   for (unsigned int i = 0; i < 3; i++) printf(" %d", props.maxThreadsDim[i]);
   printf(" )\n");

   printf("\tLocal mem size == %zu\n", props.sharedMemPerBlock);

   printf("\tGlobal mem size == %zu\n", props.totalGlobalMem);

   printf("\tConst mem size == %zu\n", props.totalConstMem);

   printf("\tRegisters per block == %d\n", props.regsPerBlock);

   printf("\tWarp size == %d\n", props.warpSize);

   printf("\n");
}

int CudaDevice::get_max_threads(){
   return device_props.maxThreadsPerBlock;
}

int CudaDevice::get_max_block_size_dimension(int dimension){
   if(dimension < 0 || dimension >= 3) return 0;
   return device_props.maxThreadsDim[dimension];
}

int CudaDevice::get_max_grid_size_dimension(int dimension){
   if(dimension < 0 || dimension >= 3) return 0;
   return device_props.maxGridSize[dimension];
}

int CudaDevice::get_warp_size(){
   return device_props.warpSize;
}

size_t CudaDevice::get_local_mem(){
   return device_props.sharedMemPerBlock;
}

}
