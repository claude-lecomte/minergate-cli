#include "miner-abstract/abstractgpuminer.h"
#include "miner-abstract/abstractgpuminer_p.h"
#include "miner-abstract/abstractgpumanager.h"

namespace MinerGate {

const quint32 HR_UPDATE_INTERVAL(5);
const quint8 DEFAULT_ITENSITY(2);

void AbstractGPUMiner::setCoinKey(const QString &_coinKey) {
  Q_ASSERT(!_coinKey.isEmpty());
  Q_D(AbstractGPUMiner);
  d->m_coinKey = _coinKey;
}

void AbstractGPUMiner::setGpuParams(const QMap<int, QVariant> &_params) {
  Q_D(AbstractGPUMiner);
  QMutexLocker l(&d->m_paramsMutex);
  if (d->m_deviceParams != _params)
    d->m_deviceParams = _params;
}

GpuMinerError AbstractGPUMiner::lastError() const {
  Q_D(const AbstractGPUMiner);
  return d->m_lastError;
}

QString AbstractGPUMiner::deviceKey() const {
  Q_D(const AbstractGPUMiner);
  return d->m_manager->deviceKey(d->m_device);
}

quint32 AbstractGPUMiner::deviceNo() const {
  Q_D(const AbstractGPUMiner);
  return d->m_device;
}

quint32 AbstractGPUMiner::takeHashCount() {
  Q_D(AbstractGPUMiner);
  quint32 result = d->m_hashCount;
  d->m_hashCount = 0;
  if (result) {
    // for this device only
    QMutexLocker l(&d->m_hrMutex);
    d->m_deviceHRCounter.addHRItem(result);
  }
  return result; // for all devices
}

qreal AbstractGPUMiner::hashrate() const {
  Q_D(const AbstractGPUMiner);
  QMutexLocker l(&d->m_hrMutex);
  return d->m_deviceHRCounter.getHashrate(HR_UPDATE_INTERVAL);
}

QString AbstractGPUMiner::coinKey() const {
  Q_D(const AbstractGPUMiner);
  return d->m_coinKey;
}

AbstractGPUMiner::AbstractGPUMiner(AbstractGPUMinerPrivate &_d):
  QObject(),
  d_ptr(&_d) {
}

AbstractGPUMiner::~AbstractGPUMiner() {
}

void AbstractGPUMiner::setTestJob(const MiningJob &_job) {
  Q_D(AbstractGPUMiner);
  d->m_testJob = _job;
}

//**************//

AbstractGPUMinerPrivate::~AbstractGPUMinerPrivate() {
}


}
