#pragma once

#include "openclminer/openclminer.h"

namespace MinerGate {

class EthOCLMinerPrivate;
class EthOCLMiner : public OpenCLMiner {
  Q_OBJECT
  Q_DECLARE_PRIVATE(EthOCLMiner)
  Q_DISABLE_COPY(EthOCLMiner)
public:
  EthOCLMiner(quint32 _device, AbstractGPUManager *_manager);
  ~EthOCLMiner();
  void start(MiningJob &_job, MiningJob &, QMutex &_jobMutex, QMutex &, volatile std::atomic<MinerThreadState> &_state, bool) Q_DECL_OVERRIDE;
  bool benchmark(qint64 &_elapsed, quint32 &_hashes) Q_DECL_OVERRIDE;
  bool allowed() const Q_DECL_OVERRIDE;
  static GpuMinerError checkCardCapability(const AbstractGPUManager *_manager, quint32 _device, const MiningJob &_job = MiningJob());
  bool checkCardCapability(const MiningJob &_job = MiningJob()) const;
};

}
