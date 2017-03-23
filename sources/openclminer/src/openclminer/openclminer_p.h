#pragma once

#include "miner-abstract/abstractgpuminer_p.h"

#include "openclminer/cltypes.h"

namespace MinerGate {

class MiningJob;
class OpenCLMinerPrivate: public AbstractGPUMinerPrivate {
public:
  OpenCLMinerPrivate(quint32 _device, AbstractGPUManager *_manager);
  ~OpenCLMinerPrivate();
  virtual bool init();
};

}
