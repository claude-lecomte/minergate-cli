#pragma once

#include "abstractminerworker.h"

namespace MinerGate {

class AbstractCPUMinerWorkerPrivate;
class AbstractCPUMinerWorker : public AbstractMinerWorker {
  Q_OBJECT
  Q_DECLARE_PRIVATE(AbstractCPUMinerWorker)
public:
  void startMining() Q_DECL_OVERRIDE;
  ~AbstractCPUMinerWorker();
protected:
  AbstractCPUMinerWorker(AbstractCPUMinerWorkerPrivate &_d);
};

}
