#include "miner-impl/ethereum/poolthread.h"
#include "miner-impl/ethereum/poolclient.h"

#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate {

class PoolThreadPrivate {
public:
  QScopedPointer<PoolClient> m_poolClient;

  QUrl m_pool_url;
  QString m_worker_name;
  QString m_password;
  QString m_log_channel;

  PoolThreadPrivate(const QUrl &_pool_url, const QString &_worker_name, const QString &_password,
                    const QString &_log_channel):
    m_pool_url(_pool_url), m_worker_name(_worker_name), m_password(_password), m_log_channel(_log_channel) {
  }
};

PoolThread::PoolThread(const QUrl &_pool_url, const QString &_worker_name, const QString &_password,
                       const QString &_log_channel) :
  QThread(),
  d_ptr(new PoolThreadPrivate(_pool_url, _worker_name, _password, _log_channel)) {
}

PoolThread::~PoolThread() {
}

void PoolThread::startListen() {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->start();
}

void PoolThread::stopListen() {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->stop();
}

void PoolThread::switchToPool(const QUrl &_pool_url) {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->switchToPool(_pool_url);
}

void PoolThread::setPool(const QUrl &_pool_url) {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->setPool(_pool_url);
}

void PoolThread::setWorkerName(const QString &_worker) {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->setWorkerName(_worker);
}

void PoolThread::submitShare(const QByteArray &_headerHash, quint64 _nonce, const QByteArray &_mixHash) {
  Q_D(PoolThread);
  if (!d->m_poolClient)
    return;
  d->m_poolClient->submitShare(_headerHash, _nonce, _mixHash);
}

void PoolThread::run() {
  Q_D(PoolThread);
  d->m_poolClient.reset(new PoolClient(d->m_pool_url, d->m_worker_name, d->m_password, d->m_log_channel));
  connect(d->m_poolClient.data(), &PoolClient::stateChangedSignal, this, &PoolThread::stateChangedSignal);
  connect(d->m_poolClient.data(), &PoolClient::newJobSignal, this, &PoolThread::newJobSignal);
  connect(d->m_poolClient.data(), &PoolClient::shareSubmittedSignal, this, &PoolThread::shareSubmittedSignal);
  connect(d->m_poolClient.data(), &PoolClient::shareErrorSignal, this, &PoolThread::shareErrorSignal);
  connect(d->m_poolClient.data(), &PoolClient::jobDifficultyChangedSignal, this, &PoolThread::jobDifficultyChangedSignal);
  exec();
  d->m_poolClient.reset();
}

}
