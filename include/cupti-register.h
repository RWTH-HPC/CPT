#ifndef CUPTI_REGISTER_H
#define CUPTI_REGISTER_H

#include <cupti.h>
#include <stdio.h>

#define CUPTI_API_CALL(apiFunctionCall)                                        \
  do {                                                                         \
    CUptiResult _status = apiFunctionCall;                                     \
    if (_status != CUPTI_SUCCESS) {                                            \
      const char *pErrorString;                                                \
      cuptiGetResultString(_status, &pErrorString);                            \
                                                                               \
      fprintf(stderr,                                                          \
              "%s:%d: Error: Function %s failed with error (%d): %s.\n",       \
              __FILE__, __LINE__, #apiFunctionCall, _status, pErrorString);    \
                                                                               \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

static void
register_cupti_runtime_callbacks(CUpti_SubscriberHandle *subscriberHandle) {
  // CUPTI_API_CALL(
  //     cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
  //         CUPTI_RUNTIME_TRACE_CBID_INVALID));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaDriverGetVersion_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaRuntimeGetVersion_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceCount_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceProperties_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaChooseDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetChannelDesc_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaCreateChannelDesc_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaConfigureCall_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaSetupArgument_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetLastError_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaPeekAtLastError_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetErrorString_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaFuncSetCacheConfig_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaFuncGetAttributes_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaSetDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaSetValidDevices_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaSetDeviceFlags_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMalloc_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMallocPitch_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cudaFree_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMallocArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFreeArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMallocHost_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFreeHost_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaHostAlloc_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaHostGetDevicePointer_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaHostGetFlags_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemGetInfo_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2D_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyArrayToArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DArrayToArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbol_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbol_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArrayAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArrayAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArrayAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArrayAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbolAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbolAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset2D_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemsetAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset2DAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetSymbolAddress_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetSymbolSize_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaBindTexture_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaBindTexture2D_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaBindTextureToArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaUnbindTexture_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureAlignmentOffset_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureReference_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaBindSurfaceToArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetSurfaceReference_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGLSetGLDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLRegisterBufferObject_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLMapBufferObject_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLUnmapBufferObject_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLUnregisterBufferObject_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLSetBufferObjectMapFlags_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLMapBufferObjectAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGLUnmapBufferObjectAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaWGLGetDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsGLRegisterImage_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsGLRegisterBuffer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsUnregisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceSetMapFlags_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsMapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsUnmapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceGetMappedPointer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsSubResourceGetMappedArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaVDPAUGetDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaVDPAUSetVDPAUDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsVDPAURegisterVideoSurface_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsVDPAURegisterOutputSurface_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDevices_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D11SetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D11RegisterResource_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDevices_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10SetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D10RegisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10RegisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10UnregisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10MapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10UnmapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceSetMapFlags_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetSurfaceDimensions_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedPointer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedSize_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10ResourceGetMappedPitch_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDevices_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9SetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9GetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsD3D9RegisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9RegisterResource_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnregisterResource_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D9MapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnmapResources_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceSetMapFlags_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetSurfaceDimensions_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedArray_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedPointer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedSize_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9ResourceGetMappedPitch_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D9Begin_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaD3D9End_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9RegisterVertexBuffer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnregisterVertexBuffer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9MapVertexBuffer_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D9UnmapVertexBuffer_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaThreadExit_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaSetDoubleForDevice_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaSetDoubleForHost_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaThreadSynchronize_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaThreadGetLimit_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaThreadSetLimit_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamCreate_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamDestroy_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamSynchronize_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamQuery_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventCreate_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEventCreateWithFlags_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventRecord_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventDestroy_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventSynchronize_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventQuery_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventElapsedTime_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMalloc3D_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMalloc3DArray_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset3D_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset3DAsync_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3D_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DAsync_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaThreadSetCacheConfig_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamWaitEvent_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D11GetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaD3D10GetDirect3DDevice_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaThreadGetCacheConfig_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaPointerGetAttributes_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaHostRegister_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaHostUnregister_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceCanAccessPeer_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceEnablePeerAccess_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceDisablePeerAccess_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaPeerRegister_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaPeerUnregister_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaPeerGetDevicePointer_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyPeer_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyPeerAsync_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeer_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeerAsync_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaDeviceReset_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSynchronize_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetLimit_v3020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetLimit_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetCacheConfig_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetCacheConfig_v3020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaProfilerInitialize_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaProfilerStart_v4000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaProfilerStop_v4000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetByPCIBusId_v4010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetPCIBusId_v4010));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGLGetDevices_v4010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaIpcGetEventHandle_v4010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaIpcOpenEventHandle_v4010));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaIpcGetMemHandle_v4010));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaIpcOpenMemHandle_v4010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaIpcCloseMemHandle_v4010));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaArrayGetInfo_v4010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaFuncSetSharedMemConfig_v4020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetSharedMemConfig_v4020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetSharedMemConfig_v4020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaCreateTextureObject_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDestroyTextureObject_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectResourceDesc_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectTextureDesc_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaCreateSurfaceObject_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDestroySurfaceObject_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetSurfaceObjectResourceDesc_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMallocMipmappedArray_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetMipmappedArrayLevel_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaFreeMipmappedArray_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaBindTextureToMipmappedArray_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceGetMappedMipmappedArray_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamAddCallback_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamCreateWithFlags_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectResourceViewDesc_v5000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetAttribute_v5000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamDestroy_v5050));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamCreateWithPriority_v5050));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetPriority_v5050));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetFlags_v5050));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetStreamPriorityRange_v5050));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMallocManaged_v6000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyMaxActiveBlocksPerMultiprocessor_v6000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamAttachMemAsync_v6000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetErrorName_v6050));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyMaxActiveBlocksPerMultiprocessor_v6050));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernel_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceFlags_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernel_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_ptds_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2D_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyArrayToArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DArrayToArray_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbol_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbol_ptds_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToArrayAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromArrayAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DToArrayAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy2DFromArrayAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyToSymbolAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyFromSymbolAsync_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset_ptds_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset2D_ptds_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemsetAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemset2DAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetPriority_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetFlags_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamSynchronize_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamQuery_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamAttachMemAsync_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaEventRecord_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemset3D_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemset3DAsync_ptsz_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3D_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamWaitEvent_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamAddCallback_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeer_ptds_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy3DPeerAsync_ptsz_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags_v7000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemPrefetchAsync_v8000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPrefetchAsync_ptsz_v8000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemAdvise_v8000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetP2PAttribute_v8000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsEGLRegisterImage_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamConsumerConnect_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamConsumerDisconnect_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamConsumerAcquireFrame_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamConsumerReleaseFrame_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamProducerConnect_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamProducerDisconnect_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamProducerPresentFrame_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamProducerReturnFrame_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphicsResourceGetMappedEglFrame_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemRangeGetAttribute_v8000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemRangeGetAttributes_v8000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEGLStreamConsumerConnectWithFlags_v7000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernel_v9000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernel_ptsz_v9000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEventCreateFromEGLSync_v9000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchCooperativeKernelMultiDevice_v9000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFuncSetAttribute_v9000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaImportExternalMemory_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaExternalMemoryGetMappedBuffer_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaExternalMemoryGetMappedMipmappedArray_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDestroyExternalMemory_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaImportExternalSemaphore_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaSignalExternalSemaphoresAsync_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaSignalExternalSemaphoresAsync_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaWaitExternalSemaphoresAsync_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaWaitExternalSemaphoresAsync_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDestroyExternalSemaphore_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaLaunchHostFunc_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchHostFunc_ptsz_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphCreate_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeGetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeSetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddKernelNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemcpyNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemcpyNodeGetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemcpyNodeSetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemsetNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemsetNodeGetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemsetNodeSetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddHostNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphHostNodeGetParams_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddChildGraphNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphChildGraphNodeGetGraph_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddEmptyNode_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphClone_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeFindInClone_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetType_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphGetRootNodes_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetDependencies_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetDependentNodes_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddDependencies_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphRemoveDependencies_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphDestroyNode_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphInstantiate_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphLaunch_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphLaunch_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecDestroy_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphDestroy_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamBeginCapture_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamBeginCapture_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamIsCapturing_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamIsCapturing_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamEndCapture_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamEndCapture_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphHostNodeSetParams_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphGetNodes_v10000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphGetEdges_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_v10010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_ptsz_v10010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecKernelNodeSetParams_v10010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaThreadExchangeStreamCaptureMode_v10010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetNvSciSyncAttributes_v10020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyAvailableDynamicSMemPerBlock_v10200));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamSetFlags_v10200));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamSetFlags_ptsz_v10200));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecMemcpyNodeSetParams_v10020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecMemsetNodeSetParams_v10020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecHostNodeSetParams_v10020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecUpdate_v10020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetFuncBySymbol_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaCtxResetPersistingL2Cache_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeCopyAttributes_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeGetAttribute_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphKernelNodeSetAttribute_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamCopyAttributes_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamCopyAttributes_ptsz_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetAttribute_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetAttribute_ptsz_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamSetAttribute_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamSetAttribute_ptsz_v11000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetTexture1DLinearMaxWidth_v11010));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphUpload_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphUpload_ptsz_v10000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemcpyNodeToSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemcpyNodeFromSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemcpyNode1D_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemcpyNodeSetParamsToSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemcpyNodeSetParamsFromSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemcpyNodeSetParams1D_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecMemcpyNodeSetParamsToSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecMemcpyNodeSetParamsFromSymbol_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecMemcpyNodeSetParams1D_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaArrayGetSparseProperties_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMipmappedArrayGetSparseProperties_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecChildGraphNodeSetParams_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddEventRecordNode_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphEventRecordNodeGetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphEventRecordNodeSetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddEventWaitNode_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphEventWaitNodeGetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphEventWaitNodeSetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecEventRecordNodeSetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecEventWaitNodeSetEvent_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEventRecordWithFlags_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaEventRecordWithFlags_ptsz_v11010));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetDefaultMemPool_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMallocAsync_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMallocAsync_ptsz_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFreeAsync_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFreeAsync_ptsz_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolTrimTo_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolSetAttribute_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolGetAttribute_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolSetAccess_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaArrayGetPlane_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolGetAccess_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolCreate_v11020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolDestroy_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetMemPool_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetMemPool_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolExportToShareableHandle_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolImportFromShareableHandle_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolExportPointer_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPoolImportPointer_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMallocFromPoolAsync_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMallocFromPoolAsync_ptsz_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaSignalExternalSemaphoresAsync_v2_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaSignalExternalSemaphoresAsync_v2_ptsz_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaWaitExternalSemaphoresAsync_v2_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaWaitExternalSemaphoresAsync_v2_ptsz_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddExternalSemaphoresSignalNode_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExternalSemaphoresSignalNodeGetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExternalSemaphoresSignalNodeSetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddExternalSemaphoresWaitNode_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExternalSemaphoresWaitNodeGetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExternalSemaphoresWaitNodeSetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecExternalSemaphoresSignalNodeSetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecExternalSemaphoresWaitNodeSetParams_v11020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceFlushGPUDirectRDMAWrites_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDriverEntryPoint_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDriverEntryPoint_ptsz_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphDebugDotPrint_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_v2_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_v2_ptsz_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamUpdateCaptureDependencies_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamUpdateCaptureDependencies_ptsz_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaUserObjectCreate_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaUserObjectRetain_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaUserObjectRelease_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphRetainUserObject_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphReleaseUserObject_v11030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphInstantiateWithFlags_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemAllocNode_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemAllocNodeGetParams_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddMemFreeNode_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphMemFreeNodeGetParams_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGraphMemTrim_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceGetGraphMemAttribute_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceSetGraphMemAttribute_v11040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeSetEnabled_v11060));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetEnabled_v11060));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaArrayGetMemoryRequirements_v11060));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMipmappedArrayGetMemoryRequirements_v11060));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernelExC_v11060));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaLaunchKernelExC_ptsz_v11060));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyMaxPotentialClusterSize_v11070));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaOccupancyMaxActiveClusters_v11070));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaCreateTextureObject_v2_v11080));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetTextureObjectTextureDesc_v2_v11080));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphInstantiateWithParams_v12000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphInstantiateWithParams_ptsz_v12000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecGetFlags_v12000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGetKernel_v12000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDeviceProperties_v2_v12000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetId_v12000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetId_ptsz_v12000));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphInstantiate_v12000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaInitDevice_v12000));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddNode_v12020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeSetParams_v12020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphExecNodeSetParams_v12020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaMemAdvise_v2_v12020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPrefetchAsync_v2_v12020));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemPrefetchAsync_v2_ptsz_v12020));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaFuncGetName_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamBeginCaptureToGraph_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamBeginCaptureToGraph_ptsz_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphConditionalHandleCreate_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphGetEdges_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetDependencies_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphNodeGetDependentNodes_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddDependencies_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGraphRemoveDependencies_v2_v12030));

  CUPTI_API_CALL(
      cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
                          CUPTI_RUNTIME_TRACE_CBID_cudaGraphAddNode_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_v3_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamGetCaptureInfo_v3_ptsz_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamUpdateCaptureDependencies_v2_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaStreamUpdateCaptureDependencies_v2_ptsz_v12030));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceRegisterAsyncNotification_v12040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaDeviceUnregisterAsyncNotification_v12040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaFuncGetParamInfo_v12040));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDriverEntryPointByVersion_v12050));

  CUPTI_API_CALL(cuptiEnableCallback(
      1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaGetDriverEntryPointByVersion_ptsz_v12050));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda470_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda471_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda472_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda473_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda474_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda475_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda476_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda477_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda478_v12060));

  CUPTI_API_CALL(cuptiEnableCallback(1, *subscriberHandle,
                                     CUPTI_CB_DOMAIN_RUNTIME_API,
                                     CUPTI_RUNTIME_TRACE_CBID_cuda479_v12060));

  // Not actual runtime calls?
  // CUPTI_API_CALL(
  //     cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
  //         CUPTI_RUNTIME_TRACE_CBID_SIZE));

  // CUPTI_API_CALL(
  //     cuptiEnableCallback(1, *subscriberHandle, CUPTI_CB_DOMAIN_RUNTIME_API,
  //         CUPTI_RUNTIME_TRACE_CBID_FORCE_INT));
}

#endif