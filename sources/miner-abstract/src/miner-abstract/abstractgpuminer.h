#pragma once
#include <stdint.h>
#include <memory>
#include <atomic>

#include "miner-abstract/miningjob.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"

#include <QScopedPointer>
#include <QMutex>
#include <QVariant>

namespace MinerGate {

class AbstractGPUMinerPrivate;
class AbstractGPUMiner: public QObject {
  Q_OBJECT
  Q_DECLARE_PRIVATE(AbstractGPUMiner)
public:
  virtual ~AbstractGPUMiner();
  void setCoinKey(const QString &_coinKey);
  virtual void start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex, QMutex &_feeJobMutex,
                     volatile std::atomic<MinerThreadState> &_state, bool _fee) = 0;
  void setGpuParams(const QMap<int, QVariant> &_params);
  GpuMinerError lastError() const;
  virtual bool benchmark(qint64 &_elapsed, quint32 &_hashCount) = 0;
  virtual bool allowed() const {return true;}
  QString deviceKey() const;
  quint32 deviceNo() const;
  quint32 takeHashCount();
  qreal hashrate() const;
  QString coinKey() const;
  void setTestJob(const MiningJob &_job);
Q_SIGNALS:
  void shareFound(const QByteArray &_jobId, const QByteArray &_hash, Nonce _nonce, bool);
  void intensityChanged(quint8 _intensity);
protected:
  QScopedPointer<AbstractGPUMinerPrivate> d_ptr;

  AbstractGPUMiner(AbstractGPUMinerPrivate &_d);
  virtual quint32 intensity2deviceFactor(quint8 _intensity) const = 0;
  virtual quint8 deviceFactor2Intensity(quint32 deviceFactor) const = 0;
};

}
