#pragma once

#include <QSharedPointer>
#include <atomic>

#include "miner-abstract/gpuminerworker.h"
#include "miner-abstract/abstractcpuminerworker.h"
#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpumanager.h"
#endif
#include "miner-impl/minerimplcommonprerequisites.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"

#include "miner-abstract/miningjob.h"
#include "miner-abstract/hrcounter.h"

#include <QTimer>
#include <QMutexLocker>

namespace MinerGate {

#ifndef Q_OS_MAC
struct GPUCtx {
  QThreadPtr thread;
  GPUMinerWorkerPtr worker;
  QString deviceKey; // key for identify card
};
typedef QSharedPointer<GPUCtx> GPUCtxPtr;
#endif

class AbstractMinerImplPrivate {
public:
  CPUWorkerType m_type;
  MiningJob m_currentJob;
  MiningJob m_feeJob;
  volatile std::atomic<quint32> m_cpuHashBuffer;

  QList<QThreadPtr> m_threads;
  QList<CPUMinerWorkerPtr> m_workers;
  HRCounter m_cpuHrCounter;
#ifndef Q_OS_MAC
  HRCounter m_gpuHrCounter;
  QMultiMap<ImplType, GPUCtxPtr> m_gpuContextList;
  QMap<ImplType, GPUManagerPtr> m_gpuManagers;
#endif
  QTimer m_hrTimer;
  QMutex m_jobMutex;
  QMutex m_fee_job_mutex;
  bool m_fee;
  QString m_coinKey;
  QVariant m_options;
  bool m_startNonceIsRandom = false;
  volatile std::atomic<bool> m_preparingInProgress;
  bool m_isReady = true;

#ifndef Q_OS_MAC
  AbstractMinerImplPrivate(CPUWorkerType _type, const QString &_coinKey, bool _fee,
                           QMap<ImplType, GPUManagerPtr> _gpuManagers,
                           const QVariant &_options = QVariant()):
    m_type(_type),
    m_gpuManagers(_gpuManagers),
    m_fee(_fee), m_coinKey(_coinKey),
    m_cpuHashBuffer(0), m_options(_options), m_preparingInProgress(false) {
  }
#else
  AbstractMinerImplPrivate(CPUWorkerType _type, const QString &_coinKey, bool _fee, const QVariant &_options = QVariant()):
    m_type(_type),
    m_fee(_fee), m_coinKey(_coinKey),
    m_cpuHashBuffer(0), m_options(_options), m_preparingInProgress(false) {
  }
#endif
};

}
