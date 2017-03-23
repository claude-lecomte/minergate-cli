#pragma once

#include <QObject>
#include "miner-abstract/abstractminerworker.h"

namespace MinerGate {

class GPUManagerPtr;
class MiningJob;
class GPUMinerWorkerPrivate;
enum class HashFamily: quint8;
enum class MinerThreadState: quint8;
class GPUMinerWorker : public AbstractMinerWorker {
  Q_OBJECT
  Q_DECLARE_PRIVATE(GPUMinerWorker)
  Q_DISABLE_COPY(GPUMinerWorker)
public:
  GPUMinerWorker(const GPUManagerPtr &_gpuManager, bool _fee, QObject* _parent, int _device, HashFamily _family);
  ~GPUMinerWorker();

  bool init();
  quint32 takeHashCount();
  void stop() Q_DECL_OVERRIDE;
  bool gpuLowMemory() const;
public Q_SLOTS:
  void updateJob(const MiningJob& _job) Q_DECL_OVERRIDE;
  void updateFeeJob(const MiningJob& _job) Q_DECL_OVERRIDE;
  void setGpuParams(const QMap<int, QVariant> &_params);
private Q_SLOTS:
  void startMining() Q_DECL_OVERRIDE;
Q_SIGNALS:
  void finishedSignal();
};

}
