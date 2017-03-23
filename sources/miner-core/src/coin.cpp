#include <QThread>
#include <QUrl>
#include <QVariantMap>
#include <QDateTime>
#include <QTimer>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "miner-core/coin.h"
#include "miner-core/coinmanager.h"
#include "miner-core/serverapi.h"
#include "miner-core/abstractminerengine.h"
#ifndef Q_OS_MAC
#include "miner-core/gpumanagercontroller.h"
#endif

#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpuminer.h"
#include "miner-abstract/abstractgpumanager.h"
#endif

#include "utils/settings.h"
#include "utils/environment.h"
#include "utils/logger.h"

namespace MinerGate {

const uint HR_UPDATE_INTERVAL(1000U);
const QString HASHRATE("hashrate");

class CoinPrivate {
public:
  CoinPrivate(const QString &_coin_key, qreal _wd_fee, qreal _wd_min, bool _custom,
               const QString &_custom_name) : m_custom(_custom),
    m_coin_key(_coin_key), m_balance(0), m_poolBalance(0),
    m_shares(0), m_badShares(0), m_eqShares(0), m_badEqShares(0), m_params(), m_cpuState(CpuMinerState::Stopped),
    m_last_error(Error::NoError), m_last_share(), m_job_difficulty(0), m_reward_method(), m_wd_fee(_wd_fee), m_wd_min(_wd_min), m_mmInProgress(0),
    m_workers_count(0), m_customName(_custom_name) {
  }

  bool m_custom;
  QString m_coin_key;
  quint64 m_balance;
  qint64 m_poolBalance;
  QPair<qreal, qreal> m_cpuHashRate = qMakePair(0,0);
#ifndef Q_OS_MAC
  QPair<qreal, qreal> m_gpuHashRate = qMakePair(0,0);
#endif
  quint64 m_shares;
  quint64 m_badShares;
  qreal m_eqShares;
  qreal m_badEqShares;
  QVariantMap m_params;
  CpuMinerState m_cpuState;
#ifndef Q_OS_MAC
  QMap<QString, GpuState> m_gpuStates;
#endif
  Error m_last_error;
  QDateTime m_last_share;
  qreal m_job_difficulty;
  QString m_reward_method;
  QTimer m_hrTimer;
  qreal m_wd_fee;
  qreal m_wd_min;
  static quint32 msUsedCores;
  quint32 m_mmInProgress;
  quint32 m_workers_count;
  QString m_customName;
  quint8 m_preparingProgress = 100;
  QString m_preparingError;
#ifndef Q_OS_MAC
  enum GPUCheckResult {NoResult, Available, NotAvailable};
  mutable GPUCheckResult m_gpuCheckResult = NoResult;
  bool m_lowGpuMemoryError = false;
#endif
};

quint32 CoinPrivate::msUsedCores = 0;

Coin::Coin(const QString &_coin_key, qreal _wd_fee, qreal _wd_min, bool _custom,
             const QString &_custom_name) : QObject(), d_ptr(new CoinPrivate(_coin_key, _wd_fee, _wd_min, _custom, _custom_name)) {
  Q_D(Coin);
  d->m_hrTimer.setInterval(HR_UPDATE_INTERVAL);

  connect(&d->m_hrTimer, &QTimer::timeout, this, [=]() {
    d->m_cpuHashRate = this->engine()->cpuHashrate();
    Q_EMIT stateChangedSignal();
#ifndef Q_OS_MAC
    d->m_gpuHashRate = this->engine()->gpuHashrate();
    Q_EMIT gpuStateChangedSignal();
#endif
  });

  connect(&ServerApi::instance(), &ServerApi::loggedOutSignal, this, &Coin::stopAll);
}

Coin::~Coin() {
}

bool Coin::custom() const {
  Q_D(const Coin);
  return d->m_custom;
}

HashFamily Coin::family() const {
  auto eng = engine();
  if (eng)
    return eng->family();
  return HashFamily::Unknown;
}

QString Coin::coinKey() const {
  Q_D(const Coin);
  return d->m_coin_key;
}

QString Coin::coinTicker() const {
  return custom() ? customName() : engine()->coinTicker();
}

QString Coin::coinName() const {
  return engine()->coinName();
}

quint8 Coin::accuracy() const {
  return engine()->accuracy();
}

bool Coin::mergedMiningPossible() const {
  Q_D(const Coin);
  return !Settings::instance().childKeys(QString("%1/%2/mergedPools").
                                         arg(d->m_custom ? "customMiners" : "miners").arg(d->m_coin_key)).isEmpty();
}

QStringList Coin::mergedMiningCurrencies() const {
  Q_D(const Coin);
  return Settings::instance().childKeys(QString("%1/%2/mergedPools").
                                        arg(d->m_custom ? "customMiners" : "miners").arg(d->m_coin_key));
}

AbstractMinerEnginePtr Coin::engine() const {
  Q_D(const Coin);
  return CoinManager::instance().engine(d->m_coin_key);
}

quint8 Coin::preparingProgress() const {
  Q_D(const Coin);
  return d->m_preparingProgress;
}

QString Coin::preparingError() const {
  Q_D(const Coin);
  return d->m_preparingError;
}

bool Coin::isReady() const {
  return engine()->isReady();
}

bool Coin::lowGpuMemory() const {
#ifndef Q_OS_MAC
  Q_D(const Coin);
  return d->m_lowGpuMemoryError;
#else
  return false;
#endif
}

QUrl Coin::currentPool() const {
  Q_D(const Coin);
  QString mm_coin = mergedMining();
  QString pool = Settings::instance().readMinerParam(d->m_coin_key,
                                                     mm_coin.isEmpty() ? "pool" : QString("mergedPools/%1").arg(mm_coin), d->m_custom).toString();
  return QUrl::fromUserInput(pool);
}

QString Coin::mergedMining() const {
  Q_D(const Coin);
  return Settings::instance().readMinerParam(coinKey(), "mergedMining", d->m_custom, QString()).toString();
}

QString Coin::worker() const {
  Q_D(const Coin);
  return d->m_custom ? Settings::instance().readMinerParam(d->m_coin_key, "login",
                                                           d->m_custom, QString()).toString() :
                       Settings::instance().readParam("email", QString()).toString();
}

quint32 Coin::cpuCores() const {
  Q_D(const Coin);
  return Settings::instance().readMinerParam(coinKey(), "speed", d->m_custom, 1).toUInt();
}

#ifndef Q_OS_MAC

bool Coin::gpuStatePresent(GpuState _state) const {
  // return true if at least one gpu device is running
  Q_D(const Coin);
  if (_state == GpuState::Stopped && d->m_gpuStates.isEmpty())
    return true; // default state is "stopped"
  Q_FOREACH (const QString &deviceKey, d->m_gpuStates.keys()) {
    if (d->m_gpuStates.value(deviceKey) == _state)
      return true;
  }
  return false;
}

quint8 Coin::gpuRunnedCount() const {
  Q_D(const Coin);
  quint8 count = 0;
  Q_FOREACH (const QString &deviceKey, d->m_gpuStates.keys())
    if (d->m_gpuStates.value(deviceKey) == GpuState::Running)
      count++;
  return count;
}

QStringList Coin::gpuRunnedDevices() const {
  Q_D(const Coin);
  QStringList list;
  Q_FOREACH (const QString &deviceKey, d->m_gpuStates.keys())
    if (d->m_gpuStates.value(deviceKey) == GpuState::Running)
      list << deviceKey;
  return list;
}
#endif

bool Coin::visible() const {
  Q_D(const Coin);
  return Settings::instance().readMinerParam(coinKey(), "visible", d->m_custom, true).toBool();
}

bool Coin::cpuRunning() const {
  Q_D(const Coin);
  return Settings::instance().readMinerParam(coinKey(), "running", d->m_custom, false).toBool();
}

#ifndef Q_OS_MAC
bool Coin::gpuRunning(const QString &_deviceKey) const {
  return Settings::instance().isGpuRunning(coinKey(), _deviceKey);
}
#endif

bool Coin::cpuAvailable() const {
  return CPUAvailable.contains(engine()->family());
}

#ifndef Q_OS_MAC
bool Coin::gpuAvailable() const {
  Q_D(const Coin);
  if (d->m_gpuCheckResult != CoinPrivate::NoResult)
    return d->m_gpuCheckResult == CoinPrivate::Available;

  if (!GPUAvailable.contains(engine()->family())) {
    d->m_gpuCheckResult = CoinPrivate::NotAvailable;
    return false;
  }

  auto checkAllIsNotAvailable = [d]() -> bool {
    if (d->m_gpuStates.isEmpty())
      return false;
    Q_FOREACH (const QString &deviceKey, d->m_gpuStates.keys()) {
      if (d->m_gpuStates.value(deviceKey) != GpuState::NotAvailable)
        return false;
    }
    return true;
  };

  int devCount = 0;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &gpuManager = CoinManager::instance().gpuManager(type);
    Q_ASSERT(gpuManager);
    for (quint32 device = 0; device < gpuManager->deviceCount(); device++)
      if (gpuManager->checkDeviceAvailable(device, engine()->family()))
        devCount++;
  }

  if (devCount && !checkAllIsNotAvailable()) {
    d->m_gpuCheckResult = CoinPrivate::Available;
    return true;
  } else {
    d->m_gpuCheckResult = CoinPrivate::NotAvailable;
    return false;
  }
}
#endif

qreal Coin::withdrawalFee() const {
  Q_D(const Coin);
  return d->m_wd_fee;
}

qreal Coin::withdrawalMin() const {
  Q_D(const Coin);
  return d->m_wd_min;
}

quint64 Coin::balance() const {
  Q_D(const Coin);
  return d->m_balance;
}

qint64 Coin::poolBalance() const {
  Q_D(const Coin);
  return d->m_poolBalance;
}

QPair<qreal, qreal> Coin::cpuHashRate() const {
  Q_D(const Coin);
  return d->m_cpuHashRate;
}

#ifndef Q_OS_MAC
QPair<qreal, qreal> Coin::gpuOverallHashRate() const {
  Q_D(const Coin);
  return d->m_gpuHashRate;
}
#endif

quint64 Coin::shares() const {
  Q_D(const Coin);
  return d->m_shares;
}

quint64 Coin::badShares() const {
  Q_D(const Coin);
  return d->m_badShares;
}

qreal Coin::eqShares() const {
  Q_D(const Coin);
  return d->m_eqShares;
}

qreal Coin::badEqShares() const {
  Q_D(const Coin);
  return d->m_badEqShares;
}

QDateTime Coin::lastShare() const {
  Q_D(const Coin);
  return d->m_last_share;
}

qreal Coin::jobDifficulty() const {
  Q_D(const Coin);
  return d->m_job_difficulty;
}

CpuMinerState Coin::cpuState() const {
  Q_D(const Coin);
  return d->m_cpuState;
}

#ifndef Q_OS_MAC
GpuState Coin::gpuState(const QString &_deviceKey) const {
  Q_D(const Coin);
  return d->m_gpuStates.value(_deviceKey);
}
#endif

Error Coin::lastError() const {
  Q_D(const Coin);
  return d->m_last_error;
}

QString Coin::rewardMethod() const {
  Q_D(const Coin);
  return d->m_reward_method;
}

quint32 Coin::workersCount() const {
  Q_D(const Coin);
  return d->m_workers_count;
}

QString Coin::customName() const {
  Q_D(const Coin);
  return d->m_customName;
}

void Coin::setMergedMining(const QString &_merged_mining_coin) {
  Q_D(Coin);
  if (mergedMining().compare(_merged_mining_coin)) {
    Settings::instance().writeMinerParam(coinKey(), "mergedMining", d->m_custom, _merged_mining_coin);
    if (d->m_cpuState == CpuMinerState::Running) {
      if (!mergedMining().isEmpty() && !CoinManager::instance().miner(mergedMining())->workersCount()) {
        CoinManager::instance().miner(mergedMining())->setWorkersCount(1);
      }

      engine()->restart();
    }

    Q_EMIT mergedMiningChangedSignal();
    CoinPtr mergedMiner = CoinManager::instance().miner(_merged_mining_coin);
  }
}

void Coin::setCpuCores(quint32 _cpu_cores) {
  Q_D(Coin);
  Q_ASSERT(_cpu_cores > 0);
  if (cpuCores() != _cpu_cores) {
    quint32 old_val = cpuCores();
    Settings::instance().writeMinerParam(coinKey(), "speed", d->m_custom, _cpu_cores);
    if (d->m_cpuState == CpuMinerState::Running) {
      CoinPrivate::msUsedCores -= old_val;
      CoinPrivate::msUsedCores += _cpu_cores;
      engine()->setCpuCores(_cpu_cores);
    }

    Q_EMIT cpuCoresChangedSignal();
  }
}

#ifndef Q_OS_MAC
void Coin::setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey) {
  Q_D(Coin);
//  if (d->m_gpuStates.value(_deviceKey) == GpuState::Running)
  engine()->setGpuParams(_params, _deviceKey);
}
#endif

void Coin::setVisible(bool _visible) {
  Q_D(Coin);
  if (visible() != _visible)
    Settings::instance().writeMinerParam(coinKey(), "visible", d->m_custom, _visible);
}

void Coin::setCpuRunning(bool _on) {
  Q_D(Coin);
  Settings::instance().writeMinerParam(coinKey(), "running", d->m_custom, _on);
}

#ifndef Q_OS_MAC
void Coin::setGpuRunning(bool _on, const QString &_deviceKey) {
  Settings::instance().setGpuRunning(coinKey(), _deviceKey, _on);
}
#endif

void Coin::setBalance(quint64 _balance, qint64 _pool_balance) {
  Q_D(Coin);
  if (d->m_balance != _balance || d->m_poolBalance != _pool_balance) {
    d->m_balance = _balance;
    d->m_poolBalance = _pool_balance;
    Q_EMIT balanceChangedSignal();
  }
}

void Coin::setShares(quint64 _shares) {
  Q_D(Coin);
  if (d->m_shares != _shares) {
    d->m_shares = _shares;
    Q_EMIT sharesChangedSignal();
  }
}

void Coin::setShares(quint64 _shares, quint64 _bad_shares, qreal _eq_shares, qreal _bad_eq_shares) {
  Q_D(Coin);
  if (d->m_shares != _shares || d->m_badShares != _bad_shares || d->m_eqShares != _eq_shares ||
      d->m_badEqShares != _bad_eq_shares) {
    d->m_shares = _shares;
    d->m_badShares = _bad_shares;
    d->m_eqShares = _eq_shares;
    d->m_badEqShares = _bad_eq_shares;
    Q_EMIT sharesChangedSignal();
  }
}

void Coin::setLastShare(const QDateTime &_last_share) {
  Q_D(Coin);
  if (d->m_last_share != _last_share) {
    d->m_last_share = _last_share;
    Q_EMIT sharesChangedSignal();
  }
}

void Coin::setJobDifficulty(qreal _job_difficulty) {
  Q_D(Coin);
  if (d->m_job_difficulty != _job_difficulty) {
    d->m_job_difficulty = _job_difficulty;
    Q_EMIT jobDifficultyChangedSignal();
  }
}

void Coin::setCpuState(CpuMinerState _state) {
  Q_D(Coin);
  if (d->m_cpuState != _state) {
    switch (_state) {
    case MinerGate::CpuMinerState::Stopped:
      if (d->m_cpuState == CpuMinerState::Running)
        CoinPrivate::msUsedCores -= cpuCores();

      break;
    case MinerGate::CpuMinerState::Running:
      if (d->m_cpuState != CpuMinerState::Running) {
        CoinPrivate::msUsedCores += cpuCores();
        QString name = custom() ? "custom" : coinTicker();
        QString label = custom() ? QString("%1:%2").arg(currentPool().host()).arg(currentPool().port()) : "";
      }

      if (d->m_workers_count == 0) {
        setWorkersCount(1);
      }

      if (!mergedMining().isEmpty() && !CoinManager::instance().miner(mergedMining())->workersCount()) {
        CoinManager::instance().miner(mergedMining())->setWorkersCount(1);
      }

      break;
    case MinerGate::CpuMinerState::Error:
      if(d->m_cpuState == CpuMinerState::Running) {
        CoinPrivate::msUsedCores -= cpuCores();
      }

      break;
    default:
      break;
    }

    if (d->m_cpuState != _state) {
      d->m_cpuState = _state;
      if (d->m_cpuState != CpuMinerState::Stopped && d->m_cpuState != CpuMinerState::MergedMining) {
        Q_EMIT cpuRunningSignal();
      } else if (d->m_cpuState != CpuMinerState::MergedMining) {
        Q_EMIT cpuStoppedSignal();
      }
    }

    Q_EMIT stateChangedSignal();
  }
}

#ifndef Q_OS_MAC
void Coin::setGpuState(GpuState _state, const QString &_deviceKey) {
  Q_D(Coin);
  auto setFn = [=](const QString &deviceKey) {
    GpuState oldState = d->m_gpuStates.value(deviceKey, GpuState::Stopped);
    if (oldState != _state) {
      d->m_gpuStates.insert(deviceKey, _state);
      Q_EMIT gpuStateChangedSignal();
      if (_state != GpuState::Stopped && _state != GpuState::Error) {
        Q_EMIT gpuRunningSignal();
        if (_state != GpuState::MergedMining) {
          QString name = custom() ? "custom" : coinTicker();
          QString label = custom() ? QString("%1:%2").arg(currentPool().host()).arg(currentPool().port()) : "";
        }
      } else if (!gpuStatePresent(GpuState::Running) && _state != GpuState::Error)
        Q_EMIT gpuStoppedSignal();

      if (_state == GpuState::Running && d->m_workers_count == 0) {
        setWorkersCount(1);
        if (!mergedMining().isEmpty() && !CoinManager::instance().miner(mergedMining())->workersCount()) {
          CoinManager::instance().miner(mergedMining())->setWorkersCount(1);
        }
      }
    }
  };

  if (_deviceKey.isEmpty())
    Q_FOREACH (const QString &key, d->m_gpuStates.keys())
      setFn(key);
  else
    setFn(_deviceKey);
}

#endif

void Coin::setLastError(MinerGate::Error _error) {
  Q_D(Coin);
  d->m_last_error = _error;
}

void Coin::setRewardMethod(const QString &_reward_method) {
  Q_D(Coin);
  d->m_reward_method = _reward_method;
  Q_EMIT rewardMethodChangedSignal();
}

void Coin::setWorkersCount(quint32 _workers_count) {
  Q_D(Coin);
  if (d->m_workers_count != _workers_count) {
    d->m_workers_count = _workers_count;
    Q_EMIT workersCountChangedSignal();
  }
}

void Coin::setWorker(const QString &_worker) {
  Q_D(Coin);
  if (d->m_custom && worker().compare(_worker)) {
    Settings::instance().writeMinerParam(d->m_coin_key, "login", d->m_custom, _worker);
  }
}

#ifndef Q_OS_MAC
void Coin::startGpuMergedMining() {
  Q_D(Coin);
  Q_FOREACH (const QString &key, d->m_gpuStates.keys())
  if (d->m_gpuStates.value(key) != GpuState::Running && d->m_gpuStates.value(key) != GpuState::NotAvailable)
      setGpuState(GpuState::MergedMining, key);
}

void Coin::stopGpuMergedMining() {
  Q_D(Coin);
  Q_FOREACH (const QString &key, d->m_gpuStates.keys())
  if (d->m_gpuStates.value(key) == GpuState::MergedMining)
    setGpuState(GpuState::Stopped, key);
}

#endif //Q_OS_MAC

void Coin::setPreparingProgress(quint8 pc) {
  Q_D(Coin);
  d->m_preparingProgress = pc;
  Q_EMIT preparingProgressChangedSignal();
}

void Coin::setPreparingError(const QString &_message) {
  Q_D(Coin);
  d->m_preparingError = _message;
  if (_message.contains("disk space"))
    d->m_last_error = Error::LowDiskSpace;
  else
    d->m_last_error = Error::PreparingError;

  if (d->m_cpuState == CpuMinerState::Running)
    setCpuState(CpuMinerState::Error);

#ifndef Q_OS_MAC
  Q_FOREACH (const QString &deviceKey, d->m_gpuStates.keys())
    if (d->m_gpuStates.value(deviceKey) == GpuState::Running)
      setGpuState(GpuState::Error, deviceKey);
#endif

  Q_EMIT lastErrorChangedSignal();
  Q_EMIT preparingErrorChangedSignal();
}

#ifndef Q_OS_MAC
void Coin::setLowGpuMemory(bool error) {
  Q_D(Coin);
  d->m_lowGpuMemoryError = error;
  Q_EMIT lowGpuMemorySignal();
}
#endif

void Coin::startCpu() {
  Q_D(Coin);
  Q_ASSERT(cpuAvailable());

  if (family() == HashFamily::Ethereum) {
    // check for only one running - eth or etc
    QString otherCoinKey;
    if (d->m_coin_key == "eth")
      otherCoinKey = "etc";
    else if (d->m_coin_key == "etc")
      otherCoinKey = "eth";
    const CoinPtr &otherMiner = CoinManager::instance().minerByCoinKey(otherCoinKey);
    if (otherMiner)
      otherMiner->stopAll();
  }

  d->m_cpuHashRate = qMakePair(0, 0);
  engine()->startCpu(cpuCores());
  d->m_hrTimer.start();
  Settings::instance().writeMinerParam(coinKey(), "running", d->m_custom, true);
}

void Coin::stopCpu(bool _sync) {
  Q_D(Coin);

  bool runningExists = false;
#ifndef Q_OS_MAC
  runningExists = gpuStatePresent(GpuState::Running);
#endif

  if (!runningExists) {
    d->m_cpuHashRate = qMakePair(0, 0);
#ifndef Q_OS_MAC
    d->m_gpuHashRate = qMakePair(0, 0);
#endif
    d->m_hrTimer.stop();
  }

  engine()->stopCpu(_sync);
  if (!_sync) {
    Settings::instance().writeMinerParam(coinKey(), "running", d->m_custom, false);
  }
}

#ifndef Q_OS_MAC
void Coin::startGpu(const QString &_deviceKey) {
  // empty _deviceKey means the all free devices
  Q_D(Coin);

  if (family() == HashFamily::Ethereum) {
    // check for only one running - eth or etc
    QString otherCoinKey;
    if (d->m_coin_key == "eth")
      otherCoinKey = "etc";
    else if (d->m_coin_key == "etc")
      otherCoinKey = "eth";
    const CoinPtr &otherMiner = CoinManager::instance().minerByCoinKey(otherCoinKey);
    if (otherMiner)
      otherMiner->stopAll();
  }

  d->m_gpuHashRate = qMakePair(0, 0);
  Q_ASSERT(gpuAvailable());

  // get available devices for family
  QStringList devicesForStart;
  if (_deviceKey.isEmpty())
    devicesForStart = GPUManagerController::allAvailableDeviceKeys(family());
  else
    devicesForStart << _deviceKey;

  // stopping this devices, if it is running on other coins
  Q_FOREACH(const QString &device, devicesForStart) {
    const CoinPtr &minerHasGpuRunned = CoinManager::instance().minerHasGpuRunned(device);
    if (minerHasGpuRunned)
      minerHasGpuRunned->stopGpu(true, device);
  }

  // starting all availble devices
  engine()->startGpu(devicesForStart);
  d->m_hrTimer.start();
}

void Coin::stopGpu(bool _sync, const QString &_deviceKey) {
  Q_D(Coin);
  d->m_gpuHashRate = qMakePair(0, 0);
  d->m_lowGpuMemoryError = false;

  if (cpuState() != CpuMinerState::Running) {
    d->m_cpuHashRate = qMakePair(0, 0);
    d->m_hrTimer.stop();
  }

  engine()->stopGpu(_sync, _deviceKey);
}
#endif

void Coin::restart() {
  engine()->restart();
}

void Coin::reset() {
  Q_D(Coin);
  d->m_cpuHashRate = qMakePair(0, 0);
#ifndef Q_OS_MAC
  d->m_gpuHashRate = qMakePair(0, 0);
#endif
  d->m_shares = 0;
  d->m_badShares = 0;
  d->m_eqShares = 0;
  d->m_badEqShares = 0;
  d->m_last_share = QDateTime();
  d->m_job_difficulty = 0;
}

QVariantMap &Coin::params() const {
  return d_ptr->m_params;
}

void Coin::stopAll() {
  stopCpu(true);
#ifndef Q_OS_MAC
  if (gpuAvailable()) {
    stopGpu(true);
  }
#endif
  reset();
}

quint32 Coin::totalCpuCores(){
  return CoinPrivate::msUsedCores;
}

QPair<qreal, QString> Coin::normalizedHashRate(qreal _hashrate) {
  QStringList items;
  items << "H/s" << "kH/s" << "MH/s" << "GH/s";
  qreal tmp_hr(_hashrate);
  QStringList::iterator it = items.begin();
  while (tmp_hr > 1000) {
    tmp_hr /= 1000.;
    ++it;
  }

  return qMakePair(tmp_hr, *it);
}

}
