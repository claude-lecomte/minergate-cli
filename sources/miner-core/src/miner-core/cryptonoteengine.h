#pragma once

#include "abstractminerengine.h"

namespace MinerGate {

enum class MinerImplState : quint8;

class _MinerCoreCommonExport CryptonoteEngine : public AbstractMinerEngine {
  Q_OBJECT
  Q_DISABLE_COPY(CryptonoteEngine)
public:
  CryptonoteEngine(HashFamily family, const QString &_coin_key, const QString &_coin_abbr, const QString &_coin_name, quint8 _accuracy);
  virtual ~CryptonoteEngine();
protected:
#ifndef Q_OS_MAC
  MinerImplPtr createMinerImpl(const QUrl &_pool, const QUrl &_devPool, const QString &_worker, const QString &_coinKey, bool _fee ,const QMap<ImplType, GPUManagerPtr> &_gpuManagers) const Q_DECL_OVERRIDE;
#else
  MinerImplPtr createMinerImpl(const QUrl &_pool, const QUrl &_devPool, const QString &_worker, const QString &_coinKey, bool _fee) const Q_DECL_OVERRIDE;
#endif
};
}
