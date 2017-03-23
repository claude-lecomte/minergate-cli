#include "miner-abstract/abstractminerworker.h"
#include "miner-abstract/abstractminerworker_p.h"

namespace MinerGate {

AbstractMinerWorker::AbstractMinerWorker(AbstractMinerWorkerPrivate &_d) :
  QObject(), d_ptr(&_d) {
  connect(this, &AbstractMinerWorker::startSignal, this, [=]() {
    startMining();
  }, Qt::QueuedConnection);
}

AbstractMinerWorker::~AbstractMinerWorker() {
}

MinerThreadState AbstractMinerWorker::getState() const {
  Q_D(const AbstractMinerWorker);
  return d->m_state;
}

void AbstractMinerWorker::start(const QString &_coinKey) {
  Q_D(AbstractMinerWorker);
  d->m_coinKey = _coinKey;
  Q_EMIT startSignal();
}

void AbstractMinerWorker::stop() {
  setState(MinerThreadState::Exit);
}

void AbstractMinerWorker::setState(MinerThreadState _state) {
  Q_D(AbstractMinerWorker);
  d->m_state = _state;
}

void AbstractMinerWorker::updateJob(const MiningJob& _job) {
  {
    Q_D(AbstractMinerWorker);
    QMutexLocker lock(&d->m_jobMutex);
    d->m_job = _job;
  }

  setState(MinerThreadState::UpdateJob);
}

void AbstractMinerWorker::updateFeeJob(const MiningJob& _job) {
  Q_D(AbstractMinerWorker);
  QMutexLocker lock(&d->m_feeJobMutex);
  d->m_feeJob = _job;
}

}
