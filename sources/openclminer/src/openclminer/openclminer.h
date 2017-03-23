#pragma once

#include "miner-abstract/abstractgpuminer.h"

namespace MinerGate {

class OpenCLMinerPrivate;
class AbstractGPUManager;
class OpenCLMiner : public AbstractGPUMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(OpenCLMiner)
public:
protected:
  OpenCLMiner(OpenCLMinerPrivate &_d);
  ~OpenCLMiner();
  quint32 intensity2deviceFactor(quint8 _intensity) const Q_DECL_OVERRIDE;
  quint8 deviceFactor2Intensity(quint32 _deviceFactor) const Q_DECL_OVERRIDE;
};

}
