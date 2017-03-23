#pragma once

#include <QObject>

#include "minercorecommonprerequisites.h"
#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpumanager.h"
#endif

namespace MinerGate {

class CoinManagerPrivate;
#ifndef Q_OS_MAC
class AbstractGPUManager;
#endif
enum class HashFamily: quint8;

class CoinManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(CoinManager)
  Q_DECLARE_PRIVATE(CoinManager)
public:
  static CoinManager &instance() {
    static CoinManager inst;
    return inst;
  }

  void init(const QVariantMap &_miners_params);
  CoinPtr addCustomMiner(const QVariantMap &_miners_params);
  void removeCustomMiner(const QString &_key);
  bool lock();

  AbstractMinerEnginePtr engine(const QString &_engine_id) const;
  const QMap<QString, CoinPtr> &miners() const;
  const QMap<QString, CoinPtr> &minersByTicker() const;
  CoinPtr miner(const QString &_coin_key) const;
  CoinPtr minerByCoinKey(const QString &_coinKey) const;
#ifndef Q_OS_MAC
  GPUManagerPtr gpuManager(ImplType _implType) const;
  CoinPtr minerHasGpuRunned(const QString _deviceKey = QString::null) const;
#endif
  QUrl devFeePool() const;
  void setDevFeePool(const QUrl &_pool_url);
  bool noLogin() const;
public Q_SLOTS:
  void exit();
private:
  QScopedPointer<CoinManagerPrivate> d_ptr;

  CoinManager();
  ~CoinManager();

  void minerStateChanged();
  void checkMergeMiningInProgress();
private Q_SLOTS:
  void loggedIn();
  void connected();

  void info(const QString &_msg);
  void error(const QString &_msg);
  void debug(const QString &_msg);
Q_SIGNALS:
  void minerAddedSignal(const QString &_coin_key);
  void minerStartedSignal(const QString &_coin_key);
  void minerStoppedSignal(const QString &_coin_key);
  void minerStateChangedSignal(const QString &_coin_key);

  void serverErrorSignal(Error _err);
  void minerErrorSignal(Error _err);
  void commonErrorSignal(Error _err);

  void otherApplicationStartedSignal();
  void mergedMiningStateChangedSignal(const QString &_merge_mining_coin, bool _on);
  void devFeePoolChangedSignal(QUrl _fee_pool);

  void initialized();
};

}
