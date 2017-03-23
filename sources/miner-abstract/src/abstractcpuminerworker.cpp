#include "miner-abstract/abstractcpuminerworker.h"
#include "miner-abstract/abstractcpuminerworker_p.h"

namespace MinerGate {

AbstractCPUMinerWorker::AbstractCPUMinerWorker(AbstractCPUMinerWorkerPrivate &_d):
  AbstractMinerWorker(_d) {
}

void AbstractCPUMinerWorker::startMining() {
  Q_D(AbstractCPUMinerWorker);
  d->startMining();
}

AbstractCPUMinerWorker::~AbstractCPUMinerWorker() {
}

}
