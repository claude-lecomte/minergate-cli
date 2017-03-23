
#pragma once

#include <QObject>
#include <QVariantMap>
#include <QUrl>

#include "miner-core/minercorecommonprerequisites.h"

namespace MinerGate
{

enum class Error : quint8;
enum class HashFamily: quint8;

class CoinPrivate;

class Coin : public QObject {
  friend class AbstractMinerEngine;
  friend class CoinManager;

  Q_OBJECT
  Q_DISABLE_COPY(Coin)
  Q_DECLARE_PRIVATE(Coin)

public:
  ~Coin();

  bool custom() const;
  HashFamily family() const;
  QString coinKey() const;
  QString coinTicker() const;
  QString coinName() const;
  quint8 accuracy() const;
  bool mergedMiningPossible() const;
  QStringList mergedMiningCurrencies() const;
  bool cpuAvailable() const;
#ifndef Q_OS_MAC
  bool gpuAvailable() const;
#endif
  qreal withdrawalFee() const;
  qreal withdrawalMin() const;
  QUrl currentPool() const;
  QString mergedMining() const;
  QString worker() const;
  quint64 balance() const;
  qint64 poolBalance() const;
  QPair<qreal, qreal> cpuHashRate() const;
#ifndef Q_OS_MAC
  QPair<qreal, qreal> gpuOverallHashRate() const;
  GpuState gpuState(const QString &_deviceKey) const;
  bool gpuStatePresent(GpuState _state) const;
  quint8 gpuRunnedCount() const;
  QStringList gpuRunnedDevices() const;
  bool gpuRunning(const QString &_deviceKey = QString::null) const;
  void setGpuState(GpuState _state, const QString &_deviceKey);
  void setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey);
  void setGpuIntensity(quint8 _intensity, const QString &_deviceKey);
  void loadSmartMiningGpuItensity();
  quint8 gpuIntensity(const QString &_deviceKey) const;
  void setGpuRunning(bool _on, const QString &_deviceKey);
  void startGpuMergedMining();
  void stopGpuMergedMining();
  void startGpu(const QString &_deviceKey = QString::null);
  void stopGpu(bool _sync = false, const QString &_deviceKey = QString::null);
#endif
  quint64 shares() const;
  quint64 badShares() const;
  qreal eqShares() const;
  qreal badEqShares() const;
  QDateTime lastShare() const;
  qreal jobDifficulty() const;
  CpuMinerState cpuState() const;
  quint32 cpuCores() const;
  MinerGate::Error lastError() const;
  QString rewardMethod() const;
  bool visible() const;
  bool cpuRunning() const;
  quint32 workersCount() const;
  QString customName() const;
  AbstractMinerEnginePtr engine() const;
  quint8 preparingProgress() const;
  QString preparingError() const;
  bool isReady() const;
  bool lowGpuMemory() const;

  void setMMInProgress(bool _on);
  static quint32 totalCpuCores();
  void setMergedMining(const QString &_merged_mining_coin);
  void setBalance(quint64 _balance, qint64 _pool_balance);
  void setShares(quint64 _shares);
  void setShares(quint64 _shares, quint64 _bad_shares, qreal _eq_shares, qreal _bad_eq_shares);
  void setLastShare(const QDateTime &_last_share);
  void setJobDifficulty(qreal _job_difficulty);
  void setCpuState(CpuMinerState _state);
  void setCpuCores(quint32 _cpu_cores);
  void setLastError(MinerGate::Error _error);
  void setRewardMethod(const QString &_reward_method);
  void setVisible(bool _visible);
  void setCpuRunning(bool _on);
  void setWorkersCount(quint32 _workers_count);
  void setWorker(const QString &_worker);
  void setPreparingProgress(quint8 pc);
  void setPreparingError(const QString &_message);
#ifndef Q_OS_MAC
  void setLowGpuMemory(bool error);
#endif

  void startCpu();
  void stopCpu(bool _sync = false);
  void restart();
  void reset();

  static QPair<qreal, QString> normalizedHashRate(qreal _hashrate);
private:
  QScopedPointer<CoinPrivate> d_ptr;

  Coin(const QString &_coin_key, qreal _wd_fee, qreal _wd_min, bool _custom = false,
    const QString &_custom_name = QString());
  QVariantMap &params() const;
private Q_SLOTS:
  void stopAll();

Q_SIGNALS:
  void mergedMiningChangedSignal();
  void balanceChangedSignal();
  void sharesChangedSignal();
  void jobDifficultyChangedSignal();
  void stateChangedSignal();
  void cpuCoresChangedSignal();
  void gpuStateChangedSignal();
  void lastErrorChangedSignal();
  void rewardMethodChangedSignal();
  void workersCountChangedSignal();
  void cpuRunningSignal(); // cpu miner runned
  void gpuRunningSignal(); // at least one device runned
  void cpuStoppedSignal(); // cpu miner stopped
  void gpuStoppedSignal(); // all device stopped
  void preparingProgressChangedSignal(); // for preparing before mining
  void preparingErrorChangedSignal();
  void lowGpuMemorySignal();
};

}
