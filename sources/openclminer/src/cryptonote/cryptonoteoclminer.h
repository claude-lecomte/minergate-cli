#pragma once

#include "openclminer/openclminer.h"

namespace MinerGate {

class CryptonoteOCLMinerPrivate;
class CryptonoteOCLMiner : public OpenCLMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CryptonoteOCLMiner)
  Q_DISABLE_COPY(CryptonoteOCLMiner)
public:
  CryptonoteOCLMiner(quint32 _device, AbstractGPUManager *_manager);
  ~CryptonoteOCLMiner();
  void start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex, QMutex &_feeJobMutex, volatile std::atomic<MinerThreadState> &_state, bool _fee) Q_DECL_OVERRIDE;
  bool benchmark(qint64 &_elapsed, quint32 &_hr) Q_DECL_OVERRIDE;
  static GpuMinerError checkCardCapability(const AbstractGPUManager *_manager, quint32 _device);
  bool allowed() const Q_DECL_OVERRIDE;
};

}
