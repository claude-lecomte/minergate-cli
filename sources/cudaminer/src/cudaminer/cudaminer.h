#pragma once

#include <QSharedPointer>
#include <atomic>

#include "miner-abstract/abstractgpuminer.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate
{

class CudaMinerPrivate;
class AbstractGPUManager;
class CudaMiner: public AbstractGPUMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CudaMiner)
  Q_DISABLE_COPY(CudaMiner)
public:
  ~CudaMiner();
protected:
  CudaMiner(CudaMinerPrivate &_d);
  quint32 intensity2deviceFactor(quint8 _intensity) const Q_DECL_OVERRIDE;
  quint8 deviceFactor2Intensity(quint32 _deviceFactor) const Q_DECL_OVERRIDE;
};

}
