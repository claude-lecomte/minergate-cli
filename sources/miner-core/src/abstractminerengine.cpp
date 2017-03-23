
#include <QUrl>
#include <QDateTime>

#include "miner-core/abstractminerengine.h"
#include "miner-core/coinmanager.h"
#include "miner-core/coin.h"
#ifndef Q_OS_MAC
  #include "miner-core/gpumanagercontroller.h"
#endif

#include "miner-impl/abstractminerimpl.h"
#include "miner-impl/minerimplcommonprerequisites.h"

#include "utils/logger.h"

namespace MinerGate
{

class AbstractMinerEnginePrivate {
public:
  AbstractMinerEnginePrivate(HashFamily _family, const QString &_coin_key, const QString &_coin_ticker,
    const QString &_coin_name, quint8 _accuracy) : m_family(_family), m_coinKey(_coin_key), m_coinTicker(_coin_ticker),
    m_coinName(_coin_name), m_accuracy(_accuracy) {
  }

  HashFamily m_family;
  QString m_coinKey;
  QString m_coinTicker;
  QString m_coinName;
  quint8 m_accuracy;
};

AbstractMinerEngine::AbstractMinerEngine(HashFamily _family, const QString &_coinKey, const QString &_coinTicker,
  const QString &_coinName, quint8 _accuracy) : QObject(),
  d_ptr(new AbstractMinerEnginePrivate(_family, _coinKey, _coinTicker, _coinName, _accuracy)) {
}

void AbstractMinerEngine::startCpuImpl(quint32 _cpu_cores) {
  const CoinPtr &miner = getMiner();
  if (!miner)
    return;

  const MinerImplPtr &minerImpl = getMinerImpl();
  minerImpl->setPool(miner->currentPool());
  minerImpl->setWorker(miner->worker());
  minerImpl->startCpu(_cpu_cores);
}

void AbstractMinerEngine::stopCpuImpl(bool _sync) {
  const MinerImplPtr &minerImpl = getMinerImpl(false);
  if (minerImpl)
    minerImpl->stopCpu(_sync);
}

#ifndef Q_OS_MAC
void AbstractMinerEngine::startGpuImpl(const QStringList &_freeDevicesKeys) {
  const CoinPtr &miner = getMiner();
  if (!miner)
    return;
  const MinerImplPtr &minerImpl = getMinerImpl();
  minerImpl->setPool(miner->currentPool());
  minerImpl->setWorker(miner->worker());
  minerImpl->startGpu(_freeDevicesKeys);
}

void AbstractMinerEngine::stopGpuImpl(bool _sync, const QString &_deviceKey) {
  const MinerImplPtr &minerImpl = getMinerImpl(false);
  if (minerImpl)
    minerImpl->stopGpu(_deviceKey, _sync);
}

#endif

void AbstractMinerEngine::restartImpl() {
  const CoinPtr &miner = getMiner();
  const MinerImplPtr &minerImpl = getMinerImpl();

  const QUrl &pool = miner->currentPool();
  Logger::debug(coinTicker(), QString("Switching to pool %2:%3").arg(pool.host()).arg(pool.port()));
  minerImpl->switchToPool(pool);
}

void AbstractMinerEngine::setCpuCoresImpl(quint32 _cpuCores) {
  Q_D(AbstractMinerEngine);
  const MinerImplPtr &minerImpl = getMinerImpl();
  minerImpl->setThreadCount(_cpuCores);
}

#ifndef Q_OS_MAC
void AbstractMinerEngine::setGpuParamsImpl(const QMap<int, QVariant> &_params, const QString &_deviceKey) {
  const MinerImplPtr &minerImpl = getMinerImpl();
  Q_ASSERT(minerImpl);
  minerImpl->setGpuParams(_params, _deviceKey);
}
#endif

QPair<qreal, qreal> AbstractMinerEngine::cpuHashrateImpl() const {
  const MinerImplPtr &minerImpl = getMinerImpl();
  if (!minerImpl)
    return qMakePair(0, 0);

  return qMakePair(minerImpl->cpuHashRate(1), minerImpl->cpuHashRate(10));
}

#ifndef Q_OS_MAC
QPair<qreal, qreal> AbstractMinerEngine::gpuHashrateImpl() const {
  const MinerImplPtr &minerImpl = getMinerImpl();
  if (!minerImpl)
    return qMakePair(0, 0);

  return qMakePair(minerImpl->gpuHashRate(5), minerImpl->gpuHashRate(60));
}
#endif

AbstractMinerEngine::~AbstractMinerEngine() {
}

void AbstractMinerEngine::startCpu(quint32 _cpu_cores) {
  startCpuImpl(_cpu_cores);
}

void AbstractMinerEngine::stopCpu(bool _sync) {
  stopCpuImpl(_sync);
}

#ifndef Q_OS_MAC
void AbstractMinerEngine::startGpu(const QStringList &_deviceKeys) {
  startGpuImpl(_deviceKeys);
}

void AbstractMinerEngine::stopGpu(bool _sync, const QString &_deviceKey) {
  stopGpuImpl(_sync, _deviceKey);
}

#endif

void AbstractMinerEngine::lowGpuMemory() const {
#ifndef Q_OS_MAC
  const CoinPtr &miner = getMiner();
  miner->setLowGpuMemory(true);
#endif
}

void AbstractMinerEngine::restart() {
  restartImpl();
}

void AbstractMinerEngine::setCpuCores(quint32 _cpu_cores) {
  setCpuCoresImpl(_cpu_cores);
}

#ifndef Q_OS_MAC
void AbstractMinerEngine::setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey) {
  setGpuParamsImpl(_params, _deviceKey);
}
#endif

QVariantMap &AbstractMinerEngine::minerParams() const {
  Q_D(const AbstractMinerEngine);
  return CoinManager::instance().miner(d->m_coinKey)->params();
}

HashFamily AbstractMinerEngine::family() const {
  Q_D(const AbstractMinerEngine);
  return d->m_family;
}

QString AbstractMinerEngine::coinKey() const {
  Q_D(const AbstractMinerEngine);
  return d->m_coinKey;
}

QString AbstractMinerEngine::coinTicker() const {
  Q_D(const AbstractMinerEngine);
  return d->m_coinTicker;
}

QString AbstractMinerEngine::coinName() const {
  Q_D(const AbstractMinerEngine);
  return d->m_coinName;
}

quint8 AbstractMinerEngine::accuracy() const {
  Q_D(const AbstractMinerEngine);
  return d->m_accuracy;
}

bool AbstractMinerEngine::isReady() const {
  const MinerImplPtr &impl = getMinerImpl(false);
  if (!impl)
    return false;
  return impl->isReady();
}

QPair<qreal, qreal> AbstractMinerEngine::cpuHashrate() const {
  return cpuHashrateImpl();
}

#ifndef Q_OS_MAC
QPair<qreal, qreal> AbstractMinerEngine::gpuHashrate() const {
  return gpuHashrateImpl();
}
#endif

CoinPtr AbstractMinerEngine::getMiner() const {
  Q_D(const AbstractMinerEngine);
  return CoinManager::instance().miner(d->m_coinKey);
}

MinerImplPtr AbstractMinerEngine::getMinerImpl(bool create) const {
  QVariantMap &params = minerParams();
  MinerImplPtr minerImpl = params.value(PAR_MINER, QVariant()).value<MinerImplPtr>();
  if (minerImpl || !create)
    return minerImpl;

  const CoinPtr &miner = getMiner();
  Q_ASSERT(miner);
  const QUrl &pool = miner->currentPool();
  Logger::debug(coinTicker(), QString("Starting on pool %2:%3").arg(pool.host()).arg(pool.port()));
#ifndef Q_OS_MAC
  QMap<ImplType, GPUManagerPtr> gpuManagers;

  if (miner->gpuAvailable())
    for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type)
      gpuManagers.insert(type, GPUManagerController::get(type));
#endif
  bool feeForCryptonoteCustomPool = miner->custom() && family() == HashFamily::CryptoNight;
  minerImpl = createMinerImpl(pool, CoinManager::instance().devFeePool(), miner->worker(),
                                 coinKey(), feeForCryptonoteCustomPool
#ifndef Q_OS_MAC
                              , gpuManagers
#endif
                              );
  params[PAR_MINER] = QVariant::fromValue(minerImpl);
  Q_ASSERT(minerParams().value(PAR_MINER, QVariant()).value<MinerImplPtr>());

  auto minerImplObj = minerImpl.data();
  connect(minerImplObj, &AbstractMinerImpl::cpuStateChangedSignal,
          this, &AbstractMinerEngine::cpuStateChanged, Qt::QueuedConnection);
#ifndef Q_OS_MAC
  connect(minerImplObj, &AbstractMinerImpl::gpuStateChangedSignal,
          this, &AbstractMinerEngine::gpuStateChanged, Qt::QueuedConnection);
#endif
  connect(minerImplObj, &AbstractMinerImpl::shareSubmittedSignal,
          this, &AbstractMinerEngine::shareSubmit);
  connect(minerImplObj, &AbstractMinerImpl::shareErrorSignal,
          this, &AbstractMinerEngine::shareError);
  connect(minerImplObj, &AbstractMinerImpl::jobDifficultyChangedSignal,
          this, &AbstractMinerEngine::jobDifficulty);
  connect(minerImplObj, &AbstractMinerImpl::preparingProgress,
          this, &AbstractMinerEngine::preparingProgressChanged);
  connect(minerImplObj, &AbstractMinerImpl::preparingError,
          this, &AbstractMinerEngine::preparingError);
  connect(minerImplObj, &AbstractMinerImpl::lowGpuMemory,
          this, &AbstractMinerEngine::lowGpuMemory);
  return minerImpl;
}

void AbstractMinerEngine::cpuStateChanged(MinerImplState _state) {
  const CoinPtr &miner = getMiner();
  CpuMinerState curState(miner->cpuState());
  const QVariantMap &params = minerParams();
  const MinerImplPtr &minerImpl = params.value(PAR_MINER, QVariant()).value<MinerImplPtr>();
  switch (_state) {
  case MinerImplState::Running:
    curState = CpuMinerState::Running;
    break;
  case MinerImplState::Stopped:
    curState = CpuMinerState::Stopped;
#ifndef Q_OS_MAC
    if (!miner->gpuRunning())
#endif
      miner->setLastShare(QDateTime());
    break;
  case MinerImplState::Error:
    curState = CpuMinerState::Error;
    if (minerImpl)
      minerImpl->switchToPool(miner->currentPool());
    break;
  default:
    break;
  }

  miner->setCpuState(curState);
}

void AbstractMinerEngine::gpuStateChanged(MinerImplState _state, const QString &_deviceKey) {
#ifndef Q_OS_MAC
  const CoinPtr &miner = getMiner();
  GpuState curState(miner->gpuState(_deviceKey));
  const QVariantMap &params = minerParams();
  const MinerImplPtr &minerImpl = params.value(PAR_MINER, QVariant()).value<MinerImplPtr>();
  switch(_state) {
  case MinerImplState::Running:
    curState = GpuState::Running;
    break;
  case MinerImplState::Stopped:
    curState = GpuState::Stopped;
    if (miner->cpuState() != CpuMinerState::Running)
      miner->setLastShare(QDateTime());
    break;
  case MinerImplState::Error:
    curState = GpuState::Error;
    if (minerImpl)
      minerImpl->switchToPool(miner->currentPool());
    break;
  default:
    break;
  }

  miner->setGpuState(curState, _deviceKey);
#else
  Q_UNUSED(_state);
  Q_UNUSED(_deviceKey);
#endif
}

void AbstractMinerEngine::shareSubmit(const QDateTime &_shareTime) {
  const CoinPtr &miner = getMiner();
  Logger::debug(coinKey(), QString("Last share: %2").arg(_shareTime.toString(Qt::ISODate)));
  miner->setLastShare(_shareTime);
  miner->setShares(miner->shares() + 1);
}

void AbstractMinerEngine::shareError() {
  const CoinPtr &miner = getMiner();
  Logger::debug(coinKey(), QString("Share error"));
  miner->setShares(miner->shares(), miner->badShares() + 1, miner->eqShares(), miner->badEqShares());
}

void AbstractMinerEngine::jobDifficulty(quint64 _jobDifficulty) {
  const CoinPtr &miner = getMiner();
  miner->setJobDifficulty(_jobDifficulty);
}

void AbstractMinerEngine::defFeePoolChanged(const QUrl &_poolUrl) {
  const QVariantMap &params = minerParams();
  const MinerImplPtr &minerImpl = params.value(PAR_MINER, QVariant()).value<MinerImplPtr>();
  if (minerImpl)
    minerImpl->switchToDevFeePool(_poolUrl);
}

void AbstractMinerEngine::preparingProgressChanged(quint8 pc) {
  const CoinPtr &miner = getMiner();
  miner->setPreparingProgress(pc);
}

void AbstractMinerEngine::preparingError(const QString &_message) const {
  const CoinPtr &miner = getMiner();
  miner->setPreparingError(_message);
}

}
