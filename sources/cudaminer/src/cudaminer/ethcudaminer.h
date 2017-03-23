#pragma once

#include "cudaminer/cudaminer.h"

namespace MinerGate {

class EthCUDAMinerPrivate;
class EthCUDAMiner : public CudaMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(EthCUDAMiner)
  Q_DISABLE_COPY(EthCUDAMiner)
public:
  EthCUDAMiner(quint32 _device, AbstractGPUManager *_manager);
  void start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex, QMutex &_feeJobMutex,
             volatile std::atomic<MinerThreadState> &_state, bool _fee) Q_DECL_OVERRIDE;
  bool benchmark(qint64 &_elapsed, quint32 &_hashCount) Q_DECL_OVERRIDE;
  bool allowed() const Q_DECL_OVERRIDE;
  static GpuMinerError checkCardCapability(const AbstractGPUManager *_manager, quint32 _device, const MiningJob &_job = MiningJob());
  bool checkCardCapability(const MiningJob &_job = MiningJob()) const;
protected:
  quint32 intensity2deviceFactor(quint8 _intensity) const Q_DECL_OVERRIDE;
  quint8 deviceFactor2Intensity(quint32 _deviceFactor) const Q_DECL_OVERRIDE;
};

}
