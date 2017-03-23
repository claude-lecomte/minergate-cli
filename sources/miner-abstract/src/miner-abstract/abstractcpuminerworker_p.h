#pragma once

#include "miner-abstract/abstractminerworker_p.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate {

class AbstractCPUMinerWorkerPrivate: public AbstractMinerWorkerPrivate {
public:
  volatile std::atomic<quint32> &m_hash_counter;
  QByteArray m_result;
  MiningJob m_currentJob;
  Nonce m_nonce = 0;

  AbstractCPUMinerWorkerPrivate(volatile std::atomic<quint32> &_hash_counter, bool _fee, QObject* _parent):
    AbstractMinerWorkerPrivate(_fee, _parent),
    m_hash_counter(_hash_counter) {
  }
  ~AbstractCPUMinerWorkerPrivate() {}

  virtual void prepareCalculation() {}
  virtual void search(Nonce startNonce, Nonce lastNonce, bool feeMode = false) = 0;
  void startMining();
};

}
