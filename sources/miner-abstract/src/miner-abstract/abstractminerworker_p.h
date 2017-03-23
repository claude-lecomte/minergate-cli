#pragma once

#include "minerabstractcommonprerequisites.h"
#include "miner-abstract/miningjob.h"
#include <atomic>
#include <QMutex>

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate {

class NonceCtl;

class AbstractMinerWorkerPrivate {
public:
  volatile std::atomic<MinerThreadState> m_state;
  QMutex m_jobMutex;
  QMutex m_feeJobMutex;
  bool m_fee;
  MiningJob m_job;
  MiningJob m_feeJob;
  QObject* m_parent;
  QString m_coinKey;

  AbstractMinerWorkerPrivate(bool _fee, QObject* _parent):
    m_state(MinerThreadState::Exit), m_fee(_fee), m_parent(_parent) {
  }
  virtual ~AbstractMinerWorkerPrivate() {}
};

}
