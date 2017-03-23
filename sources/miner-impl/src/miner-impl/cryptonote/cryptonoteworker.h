
#pragma once

#include <QObject>

#include <atomic>
#include <mutex>

#include "miner-abstract/abstractcpuminerworker.h"

namespace MinerGate {

enum class MinerThreadState : quint8;
class MiningJob;
class CryptonoteWorkerPrivate;

class CryptonoteWorker : public AbstractCPUMinerWorker {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CryptonoteWorker)
  Q_DISABLE_COPY(CryptonoteWorker)
public:
  CryptonoteWorker(volatile std::atomic<quint32> &_hash_counter, bool _fee, QObject* _parent, bool _lite);
  ~CryptonoteWorker();
};

}
