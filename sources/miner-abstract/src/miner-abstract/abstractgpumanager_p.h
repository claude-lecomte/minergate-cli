#pragma once

#include <QMap>
#include <QVariant>
#include <QString>

#include "abstractgpuminer.h"
#include "abstractgpumanager.h"

namespace MinerGate {


class AbstractGPUManagerPrivate {
public:
  QMap<quint32, QVariant> m_devicesMap;
  QMap<quint32, QPair<GPUMinerMap, bool> > m_miners;
  QString m_platformName;
  ImplType m_type;

  AbstractGPUManagerPrivate(ImplType _type):
    m_type(_type) {
  }

  virtual ~AbstractGPUManagerPrivate() {
  }
};

}
