#include <QSharedPointer>
#include <QEvent>
#include <QDateTime>
#include <QCoreApplication>

#include "miner-impl/cryptonote/cryptonoteworker.h"
#include "miner-impl/cryptonote/cryptonoteminerimpl.h"
#include "miner-impl/cryptonote/stratumclient.h"
#include "miner-impl/abstractminerimpl_p.h"
#include "miner-abstract/shareevent.h"
#include "miner-abstract/noncectl.h"

#include "utils/environment.h"
#include "utils/settings.h"


namespace MinerGate {

/* solution for cryptonotes only */
class CryptonoteMinerImplPrivate: public AbstractMinerImplPrivate {
public:
  StratumClient m_pool_client;
  StratumClient m_fee_client;
  bool m_lite;

  CryptonoteMinerImplPrivate(const QUrl &_pool_url, const QUrl &_dev_fee_pool_url, const QString &_worker,
                             const QString &_coinKey, bool _fee,
#ifndef Q_OS_MAC
                             QMap<ImplType, GPUManagerPtr> _gpuManagers,
#endif
                             bool _lite):
    AbstractMinerImplPrivate(CPUWorkerType::Cryptonote, _coinKey, _fee,
#ifndef Q_OS_MAC
                             _gpuManagers,
#endif
                             _lite),
    m_pool_client(_pool_url, _worker, QString::null, _coinKey), m_fee_client(_dev_fee_pool_url, "dev@minergate.com", "",  QString()),
    m_lite(_lite) {
  }
};

CryptonoteMinerImpl::CryptonoteMinerImpl(const QUrl &_pool_url, const QUrl &_dev_fee_pool_url, const QString &_worker,
                         const QString &_coinKey, bool _fee,
#ifndef Q_OS_MAC
                                         const QMap<ImplType, GPUManagerPtr> &_gpuManagers,
#endif
                         bool _lite):
  AbstractMinerImpl(*new CryptonoteMinerImplPrivate(_pool_url, _dev_fee_pool_url, _worker, _coinKey, _fee,
#ifndef Q_OS_MAC
                                                    _gpuManagers,
#endif
                                                    _lite)) {
  Q_D(CryptonoteMinerImpl);
  connect(&d->m_pool_client, &StratumClient::stateChangedSignal, this, &CryptonoteMinerImpl::poolClientStateChanged);
  connect(&d->m_pool_client, &StratumClient::newJobSignal, this, &CryptonoteMinerImpl::updateJob);
  connect(&d->m_fee_client, &StratumClient::newJobSignal, this, &CryptonoteMinerImpl::updateFeeJob);
  connect(&d->m_pool_client, &StratumClient::shareSubmittedSignal, this, &CryptonoteMinerImpl::shareSubmittedSignal);
  connect(&d->m_pool_client, &StratumClient::shareErrorSignal, this, &CryptonoteMinerImpl::shareErrorSignal);
  connect(&d->m_pool_client, &StratumClient::jobDifficultyChangedSignal, this, &CryptonoteMinerImpl::jobDifficultyChangedSignal);
}

CryptonoteMinerImpl::~CryptonoteMinerImpl() {
}

void CryptonoteMinerImpl::switchToPool(const QUrl &_pool_url) {
  Q_D(CryptonoteMinerImpl);
  d->m_pool_client.switchToPool(_pool_url);
}

void CryptonoteMinerImpl::switchToDevFeePool(const QUrl &_pool_url) {
  Q_D(CryptonoteMinerImpl);
  if (d->m_fee)
    d->m_fee_client.switchToPool(_pool_url);
}

void CryptonoteMinerImpl::setPool(const QUrl &_pool_url) {
  Q_D(CryptonoteMinerImpl);
  d->m_pool_client.setPool(_pool_url);
}

void CryptonoteMinerImpl::setWorker(const QString &_worker) {
  Q_D(CryptonoteMinerImpl);
  d->m_pool_client.setWorkerName(_worker);
}

void CryptonoteMinerImpl::poolStartListen() {
  Q_D(CryptonoteMinerImpl);
  d->m_pool_client.start();
  if (d->m_fee) {
      d->m_fee_client.start();
  }
}

void CryptonoteMinerImpl::poolStopListen() {
  Q_D(CryptonoteMinerImpl);
  d->m_pool_client.stop();
  d->m_fee_client.stop();
}

void CryptonoteMinerImpl::onShareEvent(ShareEvent *_event) {
  Q_D(CryptonoteMinerImpl);
  const QByteArray &jobId = _event->jobId();
  if (Q_LIKELY(!_event->fee() && jobId == d->m_currentJob.job_id)) {
    d->m_pool_client.submitShare(_event->jobId(),
                                 (quint32)_event->nonce(), _event->hash());

  } else if (_event->fee() && jobId == d->m_feeJob.job_id) {
    d->m_fee_client.submitShare(_event->jobId(),
                                (quint32)_event->nonce(), _event->hash());
  }
}

}
