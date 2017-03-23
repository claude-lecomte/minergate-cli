#include <QString>
#include <QTimer>
#include <QVariant>
#include <QUrl>
#include <QThread>
#include <QDateTime>
#include <QUuid>
#include <QUrl>

#include "miner-core/cryptonoteengine.h"

#include "miner-impl/cryptonote/cryptonoteminerimpl.h"

#define TIMER_INTERVAL 1000

namespace MinerGate {

CryptonoteEngine::CryptonoteEngine(HashFamily family, const QString &_coin_key, const QString &_coin_abbr, const QString &_coin_name, quint8 _accuracy) :
  AbstractMinerEngine(family, _coin_key, _coin_abbr, _coin_name, _accuracy) {
}

CryptonoteEngine::~CryptonoteEngine() {
}

#ifndef Q_OS_MAC
MinerImplPtr CryptonoteEngine::createMinerImpl(const QUrl &_pool, const QUrl &_devPool,
                                               const QString &_worker, const QString &_coinKey, bool _fee,
                                               const QMap<ImplType, GPUManagerPtr> &_gpuManagers
                                               ) const {
  return MinerImplPtr(new CryptonoteMinerImpl(_pool, _devPool, _worker, _coinKey, _fee, _gpuManagers, family() == HashFamily::CryptoNightLite));
}
#else
MinerImplPtr CryptonoteEngine::createMinerImpl(const QUrl &_pool, const QUrl &_devPool,
                                               const QString &_worker, const QString &_coinKey, bool _fee
                                               ) const {
  return MinerImplPtr(new CryptonoteMinerImpl(_pool, _devPool, _worker, _coinKey, _fee, family() == HashFamily::CryptoNightLite));
}
#endif

}
