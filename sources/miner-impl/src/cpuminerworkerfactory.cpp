#include "miner-impl/cpuminerworkerfactory.h"
#include "miner-impl/minerimplcommonprerequisites.h"
#include "miner-impl/cryptonote/cryptonoteworker.h"
#include "miner-impl/ethereum/ethereumworker.h"

namespace MinerGate {

CPUMinerWorkerPtr CPUMinerWorkerFactory::create(CPUWorkerType _type, volatile std::atomic<quint32> &_hash_counter,
                                                bool _fee, QObject *_parent, const QVariant &_options) {
  switch (_type) {
  case CPUWorkerType::Cryptonote:
    return CPUMinerWorkerPtr(new CryptonoteWorker(_hash_counter, _fee, _parent, _options.toBool()));
#ifndef NO_ETH
  case CPUWorkerType::Ethereum:
    return CPUMinerWorkerPtr(new EthereumWorker(_hash_counter, _fee, _parent));
#endif
  default: return CPUMinerWorkerPtr();
  }
}

}
