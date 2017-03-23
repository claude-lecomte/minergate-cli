#include "cudaminer/cudaminer.h"
#include "cudaminer/cudaminer_p.h"

#include "cuda.h"
#include "cuda_runtime.h"

namespace MinerGate {

CudaMiner::CudaMiner(CudaMinerPrivate &_d):
  AbstractGPUMiner(_d) {
}

quint32 CudaMiner::intensity2deviceFactor(quint8 _intensity) const {
  quint32 realIntensity;
  switch (_intensity) {
  case 1:
    realIntensity = 10;
    break;
  case 3:
    realIntensity = 5;
    break;
  case 4:
    realIntensity = 4;
    break;
  default: // 2
    realIntensity = 6;
    break;
  }
  return realIntensity;
}

quint8 CudaMiner::deviceFactor2Intensity(quint32 _deviceFactor) const {
  quint8 intensity;
  switch (_deviceFactor) {
  case 10:
    intensity = 1;
    break;
  case 5:
    intensity = 3;
    break;
  case 4:
    intensity = 4;
    break;
  default: // 6
    intensity = 2;
    break;
  }
  return intensity;
}

CudaMiner::~CudaMiner() {
  cudaDeviceReset();
}

}
