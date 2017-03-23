#include <QMap>
#include <QDir>
#include <QLibrary>
#include <QUrl>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QDateTime>
#include <QUuid>
#include <QLockFile>
#include <QCoreApplication>
#include <QTimer>

#include <QtCore/qmath.h>
#include <memory>

#include "utils/environment.h"
#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/familyinfo.h"

#include "miner-core/coinmanager.h"
#include "miner-core/abstractminerengine.h"
#include "miner-core/coin.h"
#include "miner-core/serverapi.h"
#include "miner-core/cryptonoteengine.h"
#ifndef Q_OS_MAC
#include "miner-core/gpumanagercontroller.h"
#endif
#ifndef NO_ETH
#include "miner-core/ethereumengine.h"
#endif

#include "miner-abstract/minerabstractcommonprerequisites.h"
#include "miner-abstract/abstractgpuminer.h"

//#define DEEP_DEBUG

namespace MinerGate {

const quint8 MAX_ACCURACY(12);

class CoinManagerPrivate {
public:
  CoinManagerPrivate() : m_lock_file(Env::dataDir().absoluteFilePath(".lock")), m_waiting_exit_or_logout(false),
    m_mine_without_login(false) {
#ifndef Q_OS_MAC
    for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type)
      m_gpuManagers.insert(type, GPUManagerController::get(type));
#endif
    m_lock_file.setStaleLockTime(500);
  }

  QMap<QString, AbstractMinerEnginePtr> m_minerEngines;
  QMap<QString, CoinPtr> m_miners;
  QMap<QString, CoinPtr> m_miners_by_ticker;
  QLockFile m_lock_file;
  QLocalServer m_srv;
  bool m_waiting_exit_or_logout;
  bool m_mine_without_login;
#ifndef Q_OS_MAC
  QMap<ImplType, GPUManagerPtr> m_gpuManagers;
#endif
  QUrl m_dev_fee_pool;
};

CoinManager::CoinManager() : QObject(), d_ptr(new CoinManagerPrivate) {
  Q_D(CoinManager);
  connect(&ServerApi::instance(), &ServerApi::loggedInSignal, this, &CoinManager::loggedIn);
  connect(&ServerApi::instance(), &ServerApi::errorSignal, this, &CoinManager::serverErrorSignal);
  connect(&d->m_srv, &QLocalServer::newConnection, this, &CoinManager::connected);
  connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CoinManager::exit);
#ifndef Q_OS_MAC
  Q_FOREACH (const GPUManagerPtr &man, d->m_gpuManagers) {
    connect(man.data(), &AbstractGPUManager::info, this, &CoinManager::info);
    connect(man.data(), &AbstractGPUManager::critical, this, &CoinManager::error);
    connect(man.data(), &AbstractGPUManager::debug, this, &CoinManager::debug);
  }
#endif
}

CoinManager::~CoinManager() {
}

void CoinManager::minerStateChanged() {
  Q_EMIT commonErrorSignal(Coin::totalCpuCores() > QThread::idealThreadCount() ? Error::CPUOverload : Error::NoError);
  checkMergeMiningInProgress();
}

void CoinManager::checkMergeMiningInProgress() {
  Q_D(CoinManager);
  QList<CoinPtr> miners = CoinManager::instance().miners().values();
  QList<QString> merged_miners;
  Q_FOREACH(const auto &miner, miners)
    merged_miners << miner->coinKey();

  Q_FOREACH(const auto &miner, miners) {
#ifndef Q_OS_MAC
    if ((miner->cpuRunning() || miner->gpuRunning()) && !miner->mergedMining().isEmpty()) {
      CoinPtr mm_miner = d->m_miners.value(miner->mergedMining());
      if (!mm_miner->cpuRunning() && !mm_miner->gpuRunning()) {
        d->m_miners.value(miner->mergedMining())->setCpuState(CpuMinerState::MergedMining);
        d->m_miners.value(miner->mergedMining())->startGpuMergedMining();
      }
      merged_miners.removeOne(miner->mergedMining());
    }
#else
    if (miner->cpuRunning() && !miner->mergedMining().isEmpty()) {
      CoinPtr mm_miner = d->m_miners.value(miner->mergedMining());
      if (!mm_miner->cpuRunning())
        d->m_miners.value(miner->mergedMining())->setCpuState(CpuMinerState::MergedMining);
      merged_miners.removeOne(miner->mergedMining());
    }
#endif
  }

  Q_FOREACH(const auto &miner, merged_miners) {
    if(d->m_miners.value(miner)->cpuState() == CpuMinerState::MergedMining)
      d->m_miners.value(miner)->setCpuState(CpuMinerState::Stopped);
#ifndef Q_OS_MAC
    d->m_miners.value(miner)->stopGpuMergedMining();
#endif
  }
}

void CoinManager::init(const QVariantMap &_miners_params) {
  Q_D(CoinManager);
  QVariantMap miner_map = Settings::instance().minerList();
  Logger::info(QString(), "Loading miners...");
  QStringList minerCoins;
  FamilyInfo::clear();
  for (auto it = _miners_params.constBegin(); it != _miners_params.constEnd(); ++it) {
    const QString &coinKey = it.key();
#ifdef NO_ETH
    if (coinKey.toLower() == "eth" || coinKey.toLower() == "etc")
      continue; // skip eth for build without ETH
#endif
    if (coinKey.isEmpty())
      continue;
    minerCoins << coinKey;
    const QVariantMap &miner_config = miner_map.value(coinKey).toMap();
    const QVariantMap &coin_map = it.value().toMap();
    const QString &coinAbbr = coin_map.value("ticker").toString();
    const QString &coinName = coin_map.value("name").toString();
    quint16 precision = coin_map.value("precision").value<quint16>();
    if (precision > MAX_ACCURACY)
      precision = MAX_ACCURACY;
    const QString &family = coin_map.value("family").toString();
    bool visible = coin_map.value("showByDefault").toBool();

    qreal wd_fee = coin_map.value("withdrawalFee").toReal();
    qreal wd_min = coin_map.value("withdrawalMin").toReal();

    const QVariantMap &pool_map = coin_map.value("server").toMap();
    QString pool_ip = pool_map.value("ip").toString();
    quint16 pool_stratum_port = pool_map.value("stratumPort").value<quint16>();

    if (coinKey.toLower() == "eth" || coinKey.toLower() == "etc") {
      // check for eth hard settings
      const QString &ethHardCodeServer = Settings::instance().readParam("eth_server").toString();
      if (!ethHardCodeServer.isEmpty())
        pool_ip = ethHardCodeServer;
      quint16 ethHardCodePort = Settings::instance().readParam(QString("%1_port").arg(coinKey)).value<quint16>();
      if (ethHardCodePort != 0)
        pool_stratum_port = ethHardCodePort;
    }

    QString merged_mining_coin;
    if (!miner_config.isEmpty()) {
      visible = miner_config.value("visible", true).toBool();
      merged_mining_coin = miner_config["mergedMining"].toString();
    }

    Settings::instance().writeMinerParam(coinKey, "visible", false, visible);

    AbstractMinerEnginePtr eng;
    if (family == FAMILY_CRYPTONIGHT)
      eng.reset(new CryptonoteEngine(HashFamily::CryptoNight, coinKey, coinAbbr, coinName, precision));
    else if (family == FAMILY_CRYPTONIGHT_LITE)
      eng.reset(new CryptonoteEngine(HashFamily::CryptoNightLite, coinKey, coinAbbr, coinName, precision));
#ifndef NO_ETH
    else if (family == FAMILY_ETHEREUM)
      eng.reset(new EthereumEngine(coinKey, coinAbbr, coinName, precision));
#endif
    else
      continue;

    d->m_minerEngines.insert(coinKey, eng);

    const QVariantMap &mm_map = pool_map.value("merged").toMap();

    QVariantMap miner_params;
    if (!mm_map.isEmpty()) {
      for (QVariantMap::const_iterator it = mm_map.begin(); it != mm_map.end(); ++it) {
        const QString &ip = it.value().toMap().value("ip").toString();
        quint16 port = it.value().toMap().value("stratumPort").value<quint16>();
        Logger::debug(QString(), QString("Adding merged pool: %1 %2:%3").arg(it.key()).arg(ip).arg(port));
        miner_params.insert(QString("mergedPools/%1").arg(it.key()), QString("stratum+tcp://%1:%2").arg(ip).arg(port));
      }

      miner_params.insert("mergedMining", merged_mining_coin);
    } else {
      miner_params.insert("mergedPools", QVariant());
      miner_params.insert("mergedMining", QVariant());
    }

    miner_params.insert("engine", coinKey);
    miner_params.insert("pool", QString("stratum+tcp://%1:%2").arg(pool_ip).arg(pool_stratum_port));
    Settings::instance().addMiner(miner_params);

    CoinPtr miner(new Coin(coinKey, wd_fee, wd_min, false));
    d->m_miners.insert(coinKey, miner);
    FamilyInfo::add(coinKey, eng->family());
    d->m_miners_by_ticker.insert(coinAbbr.toLower(), miner);

    Q_EMIT initialized();

    //stop event
    auto stopHandler = [coinKey, this]() {
    };

    connect(miner.data(), &Coin::cpuStoppedSignal, [=]() {
#ifndef Q_OS_MAC
      if (!miner->gpuRunning())
#endif
        stopHandler();
    });

#ifndef Q_OS_MAC
    connect(miner.data(), &Coin::gpuStoppedSignal, [=]() {
      if (!miner->cpuRunning())
        stopHandler();
    });
#endif
  }

  // create the custom miners
  const QVariantMap &custom_miner_map = Settings::instance().customMinerList();
  Q_FOREACH (const auto &miner_params, custom_miner_map) {
    const QVariantMap &params = miner_params.toMap();
    QMap<QString, QUrl> mm_pool_map;
    mm_pool_map.insert(QString(), params.value("pool", QVariant()).toUrl());

    const QString &sEngine = params.value("engine").toString();
    AbstractMinerEnginePtr eng(new CryptonoteEngine(HashFamily::CryptoNight, sEngine, QString(), QString(), 0));
    d->m_minerEngines.insert(sEngine, eng);

    CoinPtr miner(new Coin(sEngine, 0, 0, true, params.value("name").toString()));
    d->m_miners[params.value("engine").toString()] = miner;
    d->m_miners_by_ticker[sEngine.toLower()] = miner;
    FamilyInfo::add(miner->coinKey(), HashFamily::CryptoNight);
  }

  Q_FOREACH (const auto &miner, d->m_miners_by_ticker) {
    connect(miner.data(), &Coin::stateChangedSignal, this, &CoinManager::minerStateChanged);
    connect(miner.data(), &Coin::cpuCoresChangedSignal, this, &CoinManager::minerStateChanged);
    Q_EMIT minerAddedSignal(miner->coinKey());
  }

  Logger::info(QString(), "Miners loaded successfully");
}

CoinPtr CoinManager::addCustomMiner(const QVariantMap &_miners_params) {
  Q_D(CoinManager);
  QVariantMap params(_miners_params);

  const QString &sEngine = QUuid::createUuid().toString();
  params.insert("engine", sEngine);
  AbstractMinerEnginePtr eng;
  eng.reset(new CryptonoteEngine(HashFamily::CryptoNight, sEngine, QString(), QString(), 0));
  d->m_minerEngines.insert(sEngine, eng);

  CoinPtr miner(new Coin(sEngine, 0, 0, true, params.value("name", QString()).toString()));
  d->m_miners[sEngine] = miner;
  d->m_miners_by_ticker[sEngine.toLower()] = miner;

  FamilyInfo::add(miner->coinKey(), HashFamily::CryptoNight);
  Q_EMIT minerAddedSignal(sEngine);

  Settings::instance().addCustomMiner(params);
  return miner;
}

void CoinManager::removeCustomMiner(const QString &_key) {
  Q_D(CoinManager);
  CoinPtr miner = d->m_miners.value(_key, CoinPtr());
  if (!miner.isNull()) {
    miner->stopAll();
  }

  d->m_miners.remove(_key);
  d->m_miners_by_ticker.remove(_key);
  d->m_minerEngines.remove(_key);

  Settings::instance().removeCustomMiner(_key);
}

bool CoinManager::lock() {
  Q_D(CoinManager);
  QString unique_key("CoinManager-lock-9637ffb0-bbf9-11e3-a5e2-0800200c9a66");
  QString sock_path(Env::tempDir().absoluteFilePath(unique_key));
  if (!d->m_lock_file.tryLock()) {
    Logger::warning(QString(), "MinerGate already running");
    QLocalSocket sock;
    sock.connectToServer(sock_path);
    return !sock.waitForConnected(1000);
  }

  QLocalServer::removeServer(sock_path);
  if(!d->m_srv.listen(sock_path))
  Logger::critical(QString(), QString("Local socket listen error: %1").arg(d->m_srv.errorString()));
  return true;
}

void CoinManager::exit() {
  Q_D(CoinManager);
  Settings::instance().setAppQuitInProcess(true);
  d->m_lock_file.unlock();
  d->m_srv.close();
  Q_FOREACH(const auto &miner, d->m_miners) {
    miner->stopCpu(true);
#ifndef Q_OS_MAC
    if (miner->gpuAvailable()) {
      miner->stopGpu(true);
    }
#endif
  }
}

AbstractMinerEnginePtr CoinManager::engine(const QString &_engine_id) const {
  Q_D(const CoinManager);
  return d->m_minerEngines.value(_engine_id, AbstractMinerEnginePtr());
}

const QMap<QString, CoinPtr> &CoinManager::miners() const {
  Q_D(const CoinManager);
  return d->m_miners;
}

const QMap<QString, CoinPtr> &CoinManager::minersByTicker() const {
  Q_D(const CoinManager);
  return d->m_miners_by_ticker;
}

CoinPtr CoinManager::miner(const QString &_coin_key) const {
  Q_D(const CoinManager);
  return d->m_miners.value(_coin_key, CoinPtr());
}

CoinPtr CoinManager::minerByCoinKey(const QString &_coinKey) const {
  Q_D(const CoinManager);
  Q_FOREACH (const auto &miner, d->m_miners)
    if (!miner->coinKey().compare(_coinKey, Qt::CaseInsensitive)) {
      return miner;
    }

  return CoinPtr();
}

#ifndef Q_OS_MAC
GPUManagerPtr CoinManager::gpuManager(ImplType _implType) const {
  Q_D(const CoinManager);
  return d->m_gpuManagers.value(_implType);
}

CoinPtr CoinManager::minerHasGpuRunned(const QString _deviceKey) const {
  Q_D(const CoinManager);
  Q_FOREACH (const CoinPtr &miner, d->m_miners.values())
    if (miner->gpuRunning(_deviceKey))
      return miner;
  return CoinPtr();
}
#endif

QUrl CoinManager::devFeePool() const {
  Q_D(const CoinManager);
  return d->m_dev_fee_pool;
}

void CoinManager::setDevFeePool(const QUrl &_pool_url) {
  Q_D(CoinManager);
  if(d->m_dev_fee_pool != _pool_url) {
    d->m_dev_fee_pool = _pool_url;
    Q_EMIT devFeePoolChangedSignal(d->m_dev_fee_pool);
  }
}

bool CoinManager::noLogin() const {
  Q_D(const CoinManager);
  return d->m_mine_without_login;
}

void CoinManager::loggedIn() {
  Q_D(CoinManager);

  bool wasStart = false;
  Q_FOREACH (const auto &miner, d->m_miners) {
    if(miner->cpuRunning()) {
      miner->startCpu();
      if (!wasStart)
        wasStart = true;
    }

#ifndef Q_OS_MAC
    if (miner->gpuAvailable()) {
      for (auto implType = ImplType::FirstType; implType <= ImplType::LastType; ++implType) {
        const auto &manager = CoinManager::instance().gpuManager(implType);
        if (!manager.isNull()) {
          for (int deviceNo = 0; deviceNo < manager->deviceCount(); deviceNo++) {
            const QString &deviceKey = manager->deviceKey(deviceNo);
            if (miner->gpuRunning(deviceKey)) {
              miner->startGpu(deviceKey);
              if (!wasStart)
                wasStart = true;
            }
          }
        }
      }
    }
#endif
  }
}

void CoinManager::connected() {
  Q_EMIT otherApplicationStartedSignal();
}

void CoinManager::info(const QString &_msg) {
  Logger::info(QString(), QString("GPU: %1").arg(_msg));
}

void CoinManager::error(const QString &_msg) {
  Logger::critical(QString(), QString("GPU: %1").arg(_msg));
}

void CoinManager::debug(const QString &_msg) {
  Logger::debug(QString(), QString("GPU: %1").arg(_msg));
}

}
