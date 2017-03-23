#pragma once
#include "miner-abstract/abstractgpumanager.h"

struct _cl_context;
namespace MinerGate {

class OCLDevicePtr;
class OpenCLManagerPrivate;
class OpenCLManager : public AbstractGPUManager {
  Q_OBJECT
  Q_DECLARE_PRIVATE(OpenCLManager)
  friend class OpenCLMiner;
public:
  OpenCLManager();
  QString deviceName(quint32 _device) const Q_DECL_OVERRIDE;
  bool checkDeviceAvailable(quint32 _device, HashFamily _family) const Q_DECL_OVERRIDE;
protected:
  GPUCoinPtr createMiner(quint32 _device, HashFamily _family) Q_DECL_OVERRIDE;
};

}
