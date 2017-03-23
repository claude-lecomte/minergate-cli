#pragma once

#include "miner-abstract/abstractgpumanager.h"
#include "miner-abstract/abstractgpuminer_p.h"

#include "cudadeviceprops.h"


namespace MinerGate {

class CudaMinerPrivate: public AbstractGPUMinerPrivate {
public:
  CudaMinerPrivate(quint32 _device, AbstractGPUManager *_manager) :
    AbstractGPUMinerPrivate(_device, _manager) {
  }

  // WARNING: check this for new versions of CUDA
  static size_t cudaCoresPerMP(int _major, int _minor) {
    size_t res = 0;
    switch(_major) {
    case 1:
      res = 8;
      break;
    case 2:
      switch(_minor) {
      case 0:
        res = 32;
        break;
      case 1:
        res = 48;
        break;
      }
      break;
    case 3:
      res = 192;
      break;
    default:
      res = 128; // 5 and more
      break;
    }
    return res;
  }

  quint32 calcBlockSize(quint32 _threads) const {
    const CudaDevicePropPtr &devProps = m_manager->deviceProps(m_device).value<CudaDevicePropPtr>();
    Q_ASSERT(devProps);
    int cudaCores = devProps->multiProcessorCount * cudaCoresPerMP(devProps->major, devProps->minor);
    return cudaCores / _threads;
  }

  ~CudaMinerPrivate() {
  }
};

}
