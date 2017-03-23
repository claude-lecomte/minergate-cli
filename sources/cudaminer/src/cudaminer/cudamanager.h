#pragma once

#include "miner-abstract/abstractgpumanager.h"

namespace MinerGate {

class CudaManagerPrivate;
class CudaManager: public AbstractGPUManager {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CudaManager)
  Q_DISABLE_COPY(CudaManager)
public:
  CudaManager();
  ~CudaManager();
  QString deviceName(quint32 _device) const Q_DECL_OVERRIDE;
  bool checkDeviceAvailable(quint32 _device, HashFamily _family) const Q_DECL_OVERRIDE;
protected:
  GPUCoinPtr createMiner(quint32 _device, HashFamily family) Q_DECL_OVERRIDE;
private:
};

}
