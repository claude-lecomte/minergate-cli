#include "miner-core/ethereumengine.h"
#include "miner-core/coinmanager.h"
#include "miner-core/coin.h"

#include "miner-core/minercorecommonprerequisites.h"
#include "miner-impl/ethereum/ethereumminerimpl.h"

namespace MinerGate {

EthereumEngine::EthereumEngine(const QString &_coin_key, const QString &_coin_abbr, const QString &_coin_name, quint8 _accuracy):
  AbstractMinerEngine(HashFamily::Ethereum, _coin_key, _coin_abbr, _coin_name, _accuracy) {
}

EthereumEngine::~EthereumEngine() {
}

#ifndef Q_OS_MAC
MinerImplPtr EthereumEngine::createMinerImpl(const QUrl &_pool, const QUrl &, const QString &_worker, const QString &_coinKey, bool _fee,
                                                        const QMap<ImplType, GPUManagerPtr> &_gpuManagers) const {
  return MinerImplPtr(new EthereumMinerImpl(_pool, _worker, _coinKey, _fee, _gpuManagers));
}
#else
MinerImplPtr EthereumEngine::createMinerImpl(const QUrl &_pool, const QUrl &, const QString &_worker, const QString &_coinKey, bool _fee) const {
  return MinerImplPtr(new EthereumMinerImpl(_pool, _worker, _coinKey, _fee));
}
#endif

}
