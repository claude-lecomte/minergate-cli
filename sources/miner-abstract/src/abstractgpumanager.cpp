#include "miner-abstract/abstractgpumanager.h"
#include "miner-abstract/abstractgpumanager_p.h"
#include "miner-abstract/abstractgpuminer.h"

#include "utils/familyinfo.h"

#include <QCoreApplication>

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate {

AbstractGPUManager::~AbstractGPUManager() {
}

quint8 AbstractGPUManager::deviceCount() const {
  Q_D(const AbstractGPUManager);
  return d->m_devicesMap.count();
}

quint8 AbstractGPUManager::deviceCount(HashFamily _family) const {
  Q_D(const AbstractGPUManager);
  if (availableForFamily(_family))
    return d->m_devicesMap.count();
  else
    return 0;
}

QVariant AbstractGPUManager::deviceProps(quint32 _device) const {
  Q_D(const AbstractGPUManager);
  return d->m_devicesMap.value(_device);
}

quint32 AbstractGPUManager::deviceNo(const QString &deviceKey) {
  const QStringList &parts = deviceKey.split('_');
  Q_ASSERT(parts.count() == 2);
  return parts.last().toUInt();
}

GPUCoinPtr AbstractGPUManager::getMiner(quint32 _device, HashFamily _family, bool create) {
  if(!GPUAvailable.contains(_family))
    return GPUCoinPtr();
  Q_D(AbstractGPUManager);
  if (_device >= d->m_devicesMap.size())
    return GPUCoinPtr();
  GPUMinerMap &minerMap = d->m_miners[_device].first;
  GPUCoinPtr &miner = minerMap[_family];
  if (!miner) {
    if (!create)
      return GPUCoinPtr();
    // miner will be created for the available device only
    if (checkDeviceAvailable(_device, _family))
      miner = createMiner(_device, _family);
    if (!miner)
      return GPUCoinPtr();
    Q_ASSERT(d->m_miners.value(_device).first.contains(_family));
  }
  d->m_miners[_device].second = true;
  return miner;
}

void AbstractGPUManager::freeMiner(quint32 _device) {
  Q_D(AbstractGPUManager);
  if (_device >= d->m_devicesMap.size())
    return;
  d->m_miners[_device].second = false;
}

void AbstractGPUManager::message(AbstractGPUManager::MessageType _type, const QString &_msg) {
  switch (_type) {
  case Info: Q_EMIT info(_msg); break;
  case Critical: Q_EMIT critical(_msg); break;
  case Debug: Q_EMIT debug(_msg); break;
  }
}

QString AbstractGPUManager::deviceKey(quint32 _device) const {
  Q_D(const AbstractGPUManager);
  return QString("%1_%2").arg((int)d->m_type).arg(_device);
}

ImplType AbstractGPUManager::type() const {
  Q_D(const AbstractGPUManager);
  return d->m_type;
}

bool AbstractGPUManager::availableForFamily(HashFamily _family) const {
  return GPUAvailable.contains(_family);
}

AbstractGPUManager::AbstractGPUManager(MinerGate::AbstractGPUManagerPrivate &d):
  QObject(), d_ptr(&d) {}

}
