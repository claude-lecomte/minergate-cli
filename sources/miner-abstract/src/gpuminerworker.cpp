#include <QCoreApplication>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "miner-abstract/minerabstractcommonprerequisites.h"
#include "miner-abstract/gpuminerworker.h"
#include "miner-abstract/abstractgpuminer.h"
#include "miner-abstract/abstractgpumanager.h"
#include "miner-abstract/abstractminerworker_p.h"
#include "miner-abstract/shareevent.h"

#include "utils/logger.h"

namespace MinerGate {

class GPUMinerWorkerPrivate: public AbstractMinerWorkerPrivate {
public:
  GPUMinerWorkerPrivate(const GPUManagerPtr &_gpuManager, bool _fee, QObject* _parent) :
    AbstractMinerWorkerPrivate(_fee, _parent), m_gpuManager(_gpuManager)/*, m_intensity(0)*/ {
  }
  ~GPUMinerWorkerPrivate() {}

  GPUManagerPtr m_gpuManager;
  GPUCoinPtr m_gpuMiner;
//  quint8 m_intensity;
};

GPUMinerWorker::GPUMinerWorker(const GPUManagerPtr &_gpuManager, bool _fee, QObject* _parent, int _device, HashFamily _family):
  AbstractMinerWorker(*new GPUMinerWorkerPrivate(_gpuManager, _fee, _parent)) {
  Q_D(GPUMinerWorker);
  d->m_gpuMiner = d->m_gpuManager->getMiner(_device, _family);
  Q_ASSERT(d->m_gpuMiner);
  connect(d->m_gpuMiner.data(), &AbstractGPUMiner::shareFound,
    this, [d](const QByteArray &_jobId, const QByteArray &_hash, Nonce _nonce, bool _fee) {
    QCoreApplication::postEvent(d->m_parent, new ShareEvent(_jobId, _nonce, _hash, _fee), Qt::HighEventPriority);
  });
}

GPUMinerWorker::~GPUMinerWorker() {
}

bool GPUMinerWorker::init() {
  Q_D(GPUMinerWorker);
  return d->m_gpuMiner != nullptr;
}

quint32 GPUMinerWorker::takeHashCount() {
  Q_D(GPUMinerWorker);
  return d->m_gpuMiner->takeHashCount();
}

void GPUMinerWorker::startMining() {
  Q_D(GPUMinerWorker);
  if (!d->m_gpuMiner || d->m_state == MinerThreadState::Stopping) {
    return;
  }

  setState(MinerThreadState::UpdateJob);
  Logger::debug(QString::null, "GPU miner started, coin: " + d->m_coinKey);
  d->m_gpuMiner->setCoinKey(d->m_coinKey);
  d->m_gpuMiner->start(d->m_job, d->m_feeJob, d->m_jobMutex, d->m_feeJobMutex,
                       d->m_state, d->m_fee);
  if (d->m_state == MinerThreadState::Stopping)
    qApp->processEvents(QEventLoop::EventLoopExec); // prevent run start again
  setState(MinerThreadState::Exit);
  GpuMinerError error = d->m_gpuMiner->lastError();
  if (error != GpuMinerError::NoError) {
    QString msg;
    switch (error) {
    case GpuMinerError::NoMemory:
      msg = tr("Low GPU memory");
      break;
    default: msg = tr("Unknown error"); break;
    }
    Logger::critical(QString::null, msg);
  }
  Logger::info(QString::null, "GPU miner finished");
  Q_EMIT finishedSignal();
}

void GPUMinerWorker::stop() {
  setState(MinerThreadState::Stopping);
}

bool GPUMinerWorker::gpuLowMemory() const {
  Q_D(const GPUMinerWorker);
  return d->m_gpuMiner && d->m_gpuMiner->lastError() == GpuMinerError::NoMemory;
}

void GPUMinerWorker::updateJob(const MiningJob& _job) {
  Q_D(GPUMinerWorker);
  if (d->m_state != MinerThreadState::Mining && d->m_state != MinerThreadState::UpdateJob)
    return;

  AbstractMinerWorker::updateJob(_job);
}

void GPUMinerWorker::updateFeeJob(const MiningJob& _job) {
  Q_D(GPUMinerWorker);
  if (d->m_state != MinerThreadState::Mining)
    return;

  AbstractMinerWorker::updateFeeJob(_job);
}

void GPUMinerWorker::setGpuParams(const QMap<int, QVariant> &_params) {
  Q_D(GPUMinerWorker);
  d->m_gpuMiner->setGpuParams(_params);
}

}
