#pragma once
#include "miner-abstract/hrcounter.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"
#include "miner-abstract/miningjob.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <atomic>
#include <QMutex>
#include <QMap>
#include <QVariant>


namespace MinerGate {

class AbstractGPUManager;
class AbstractGPUMinerPrivate {
public:
  quint32 m_device;
  mutable QMutex m_paramsMutex;
  QMap<int, QVariant> m_deviceParams;
  volatile std::atomic<quint32> m_hashCount;
  mutable GpuMinerError m_lastError = GpuMinerError::NoError;
  AbstractGPUManager *m_manager;
  mutable QMutex m_hrMutex;
  HRCounter m_deviceHRCounter;
  QString m_coinKey;
  MiningJob m_testJob;

  AbstractGPUMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
    m_device(_device), m_manager(_manager) {
    m_hashCount = 0;
  }

  virtual ~AbstractGPUMinerPrivate();
};

}

