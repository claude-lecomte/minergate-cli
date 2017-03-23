#include "cudaminer/cudamanager.h"
#include "cudaminer/cudadeviceprops.h"
#include "cudaminer/cryptonotecudaminer.h"
#include "cudaminer/cudadeviceprops.h"
#include "cudaminer/cryptonotecudaminer.h"

#ifndef NO_ETH
  #include "cudaminer/ethcudaminer.h"
#endif

#include "miner-abstract/abstractgpumanager_p.h"

#include "utils/logger.h"
#include "utils/familyinfo.h"

namespace MinerGate {

class CudaManagerPrivate: public AbstractGPUManagerPrivate {
public:
  CudaManagerPrivate():
    AbstractGPUManagerPrivate(ImplType::CUDA) {
    m_devicesMap = cudaDevices();
  }

private:
  static quint32 cudaDevicesCount() {
    int version;
    cudaError err = cudaDriverGetVersion(&version);
    if (err != cudaSuccess)
      return 0;

    int maj = version / 1000, min = version % 100; // same as in deviceQuery sample
    if (maj < 5 || (maj == 5 && min < 5))
      return 0;

    int devNumber;
    err = cudaGetDeviceCount(&devNumber);
    if (err != cudaSuccess) {
      Logger::critical(QString::null, QString("CUDA init error: %1").arg(err));
      return 0;
    }
    return devNumber;
  }

  static QMap<quint32, QVariant> cudaDevices() throw() {
    quint32 devNumber = cudaDevicesCount();
    QMap<quint32, QVariant> res;
    for (quint32 i = 0; i < devNumber; ++i) {
      CudaDevicePropPtr props(new cudaDeviceProp);
      if (cudaGetDeviceProperties(static_cast<cudaDeviceProp*>(props.data()), i) == cudaSuccess)
        res.insert(i, QVariant::fromValue(props));
    }
    return res;
  }
};

CudaManager::CudaManager() :
  AbstractGPUManager(*new CudaManagerPrivate) {
}

QString CudaManager::deviceName(quint32 _device) const {
  Q_D(const CudaManager);
  const QVariant &val = d->m_devicesMap.value(_device);
  if (val.isNull())
    return QString();
  return val.value<CudaDevicePropPtr>()->name;
}

GPUCoinPtr CudaManager::createMiner(quint32 _device, HashFamily family) {
  GPUCoinPtr miner;
  switch (family) {
  case HashFamily::CryptoNight:
    miner.reset(new CryptonoteCudaMiner(_device, this));
    break;
#ifndef NO_ETH
  case HashFamily::Ethereum:
    miner.reset(new EthCUDAMiner(_device, this));
    break;
#endif
  default: return GPUCoinPtr();
  }
  // check for mining capability on this card
  if (miner->allowed())
    return miner;
  else
    return GPUCoinPtr();
}

CudaManager::~CudaManager() {
}

bool CudaManager::checkDeviceAvailable(quint32 _device, HashFamily _family) const {
  // precheck mining capability
  switch (_family) {
  case HashFamily::CryptoNight:
    return true; //TODO add conditions
#ifndef NO_ETH
  case HashFamily::Ethereum:
    return EthCUDAMiner::checkCardCapability(this, _device) == GpuMinerError::NoError;
#endif
  default:
    return GPUCoinPtr();
  }
}

}


