#pragma once

#include "miner-abstract/abstractcpuminerworker.h"

#include <atomic>

namespace MinerGate {

class EthereumWorkerPrivate;
class EthereumWorker: public AbstractCPUMinerWorker {
  Q_OBJECT
  Q_DECLARE_PRIVATE(EthereumWorker)
public:
  EthereumWorker(volatile std::atomic<quint32> &_hashCounter, bool _fee, QObject* _parent);
  ~EthereumWorker();
};

}
