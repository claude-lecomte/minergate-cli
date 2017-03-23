#pragma once

#include "cudaminer/cudaminer.h"

namespace MinerGate {

class CryptonoteCudaMinerPrivate;
class CryptonoteCudaMiner : public CudaMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CryptonoteCudaMiner)
  Q_DISABLE_COPY(CryptonoteCudaMiner)
public:
  CryptonoteCudaMiner(quint32 _device, AbstractGPUManager *_manager);
  ~CryptonoteCudaMiner();
  void start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex, QMutex &_feeJobMutex,
             volatile std::atomic<MinerThreadState> &_state, bool _fee) Q_DECL_OVERRIDE;
  bool benchmark(qint64 &_elapsed, quint32 &_hashCount) Q_DECL_OVERRIDE;
};

}
