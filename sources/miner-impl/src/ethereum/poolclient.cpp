#include <QJsonObject>
#include <QJsonDocument>
#include <QQueue>
#include <QUuid>
#include <QTimer>
#include <QDataStream>
#include <QDateTime>
#include <QTcpSocket>

#include <cstring>

#include "miner-impl/ethereum/ethstratumadapter.h"
#include "miner-impl/ethereum/poolclient.h"

#include "networkutils/downloadmanager.h"
#include "networkutils/networkenvironment.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/environment.h"

#include "miner-abstract/miningjob.h"

#include "qjsonrpc/qjsonrpcclient.h"

#include "libdevcore/FixedHash.h"

namespace MinerGate {

enum PoolParams :quint8 {PHeaderHash, PSeedHash, PBoudary, PWorkerId, PLast = PWorkerId};
const quint32 POOL_LOGIN_INTERVAL(3000);

class PoolClientPrivate {
public:
  PoolClientPrivate(const QString &_worker, const QString &_password,
    const QString &_log_channel) : m_worker(_worker), m_password(_password), m_currentWorkerId(),
    m_logChannel(_log_channel) {
  }

  QString m_worker;
  QString m_password;
  QString m_currentWorkerId;
  QTimer m_loginTimer;
  QString m_logChannel;
  EthStratumAdapter m_adapter;

  static inline quint32 getDifficultyForBoundary(const dev::h256 &_boundary) {
    using namespace dev;
    const u256 diff((bigint(1) << 256) / (const u256)_boundary);
    return diff.convert_to<quint32>();
  }
};

PoolClient::PoolClient(const QUrl &_pool_url, const QString &_worker, const QString &_password,
  const QString &_log_channel, QObject *) : JsonRpc::JsonRpcClient(_pool_url),
  d_ptr(new PoolClientPrivate(_worker, _password, _log_channel)) {
  Q_D(PoolClient);
  d->m_loginTimer.setInterval(POOL_LOGIN_INTERVAL);
  setAdapter(&d->m_adapter);

  connect(&d->m_adapter, &EthStratumAdapter::jsonrpc_response_login, this, &PoolClient::login);
  connect(&d->m_adapter, &EthStratumAdapter::jsonrpc_response_eth_getWork, this, &PoolClient::getjob);
  connect(&d->m_adapter, &EthStratumAdapter::jsonrpc_notification_eth_work, this, &PoolClient::getjob);
  connect(&d->m_adapter, &EthStratumAdapter::jsonrpc_response_eth_submitWork, this, &PoolClient::submit);
  connect(&d->m_adapter, &JsonRpc::JsonRpcAdapter::jsonrpc_error, this, &PoolClient::error);

  connect(this, &JsonRpcClient::connected, this, &PoolClient::tryLogin);
  connect(this, &JsonRpcClient::socketErrorSignal, this, &PoolClient::socketError);
  connect(&NetworkEnvironment::instance(), SIGNAL(updateProxySignal()), SLOT(updateProxy()));
  connect(&d->m_loginTimer, &QTimer::timeout, this, &PoolClient::tryLogin);
}

PoolClient::~PoolClient() {
}

void PoolClient::start() {
  Q_D(PoolClient);
  if (!d->m_currentWorkerId.isNull()) {
    return;
  }

  connectToHost();
}

void PoolClient::stop() {
  Q_D(PoolClient);
  d->m_loginTimer.stop();
  disconnectFromHost();
  d->m_currentWorkerId = QString();
  Logger::info(d->m_logChannel, "Ethereum pool client stopped");
  Q_EMIT jobDifficultyChangedSignal(0);
}

void PoolClient::switchToPool(const QUrl &_pool_url) {
  if (_pool_url != url()) {
    stop();
    setUrl(_pool_url);
    start();
  }
}

void PoolClient::setPool(const QUrl &_pool_url) {
  setUrl(_pool_url);
}

void PoolClient::setWorkerName(const QString &_worker) {
  Q_D(PoolClient);
  Q_ASSERT(!_worker.isEmpty());
  d->m_worker = _worker;
}

void PoolClient::tryLogin() {
  Q_D(PoolClient);
  QVariantMap params_map;
  params_map.insert("agent", Env::userAgent());
  Q_ASSERT(!d->m_worker.isEmpty());
  params_map.insert("login", d->m_worker);
  params_map.insert("pass", d->m_password);
  d->m_adapter.jsonrpc_request_login(params_map);
}

void PoolClient::updateJob() {
  Q_D(PoolClient);
  QVariantMap params_map;
  params_map.insert("id", d->m_currentWorkerId);
  d->m_adapter.jsonrpc_request_eth_getWork(params_map);
}

void PoolClient::submitShare(const QByteArray &_headerHash, quint64 _nonce, const QByteArray &_mixHash) {
  Q_D(PoolClient);
  if (d->m_currentWorkerId.isEmpty()) {
    return;
  }

  QVariantList params;

  QByteArray nonce_arr;
  {
    QDataStream nonce_str(&nonce_arr, QIODevice::WriteOnly);
    nonce_str.setByteOrder(QDataStream::BigEndian);
    nonce_str << _nonce;
  }

  params << QString::fromUtf8(nonce_arr.toHex()) << _headerHash.toHex()
         << _mixHash.toHex() << d->m_currentWorkerId;

  d->m_adapter.jsonrpc_request_eth_submitWork(params);

  Logger::debug(d->m_logChannel, QString("Submitting share!!! sessionId=\"%1\" nonce=\"%2\" mixHash=\"%3\"")
                .arg(d->m_currentWorkerId)
                .arg(QString::fromUtf8(nonce_arr.toHex()))
                .arg(QString::fromUtf8(_mixHash.toHex())));
}

void PoolClient::setNewJob(const QVariantList &_jobParams) {
  Q_D(PoolClient);
  const QByteArray headerHash = QByteArray::fromHex(_jobParams.at(PHeaderHash).toByteArray());

  const QByteArray seedHash = QByteArray::fromHex(_jobParams.at(PSeedHash).toByteArray());
  // end hard code
  const QByteArray boundary = QByteArray::fromHex(_jobParams.at(PBoudary).toByteArray());
  int workerId = 0;
  if (_jobParams.count() - 1 >= PWorkerId)
    workerId = _jobParams.at(PWorkerId).toInt();

  if (!Env::consoleApp() || Env::verbose())
    Logger::info(d->m_logChannel, QString("New Job: seedHash=\"%1\" headerHash=\"%2\" boundary=\"%3\"")
                 .arg(QString::fromUtf8(seedHash.toHex()))
                 .arg(QString::fromUtf8(headerHash.toHex()))
                 .arg(QString::fromUtf8(boundary.toHex())));
  if (workerId)
    d->m_currentWorkerId = workerId;
  MiningJob job(headerHash, seedHash, boundary);
  reset(false);

  Q_EMIT newJobSignal(job);

  //calculate the difficulty
  quint32 jobDiff = d->getDifficultyForBoundary(dev::arrayToH256(boundary));
  Q_EMIT jobDifficultyChangedSignal(jobDiff);
}

void PoolClient::login(QVariantList _params) {
  Q_D(PoolClient);
  Q_EMIT stateChangedSignal(PoolState::Connected);
  d->m_loginTimer.stop();
  d->m_currentWorkerId = _params.at(PWorkerId).toString();
  Logger::info(d->m_logChannel, QString("Successfully connected to pool: %1. worker_id=\"%2\"").
    arg(url().toString()).arg(d->m_currentWorkerId));
  setNewJob(_params);
}

void PoolClient::getjob(QVariantList _params) {
  setNewJob(_params);
}

void PoolClient::submit(bool result) {
  Q_D(PoolClient);
  if (result) {
    if (Env::consoleApp() && Env::verbose()) {
      Logger::info(d->m_logChannel, QString("Share submitted successfully!!!"));
    } else {
      Logger::debug(d->m_logChannel, QString("Share submitted successfully!!!"));
    }

    Q_EMIT shareSubmittedSignal(QDateTime::currentDateTime());
  }
}

void PoolClient::job(QVariantList _params) {
  Logger::debug(QString(), "New JOB notification");
  setNewJob(_params);
}

void PoolClient::error(qint32 _code, QString _message, QByteArray _data, QVariantMap _request) {
  Q_D(PoolClient);
  switch (static_cast<EthereumPoolError>(_code)) {
  case EthereumPoolError::NoError:
    break;
  case EthereumPoolError::DuplicateShare:
  case EthereumPoolError::InvalidShare:
  case EthereumPoolError::StaleJob:
  case EthereumPoolError::StaleJob2:
  case EthereumPoolError::LowDifficultyShare:
    Logger::critical(d->m_logChannel, QString("Failed to submit share! (msg=\"%1\" id=\"%2\" nonce=\"%3\" result=\"%4\"), updating job...").
      arg(_message).
      arg(_request.value("params").toMap().value("id").toString()).
      arg(_request.value("params").toMap().value("nonce").toString()).
      arg(_request.value("params").toMap().value("result").toString()));

    reset(false);
    Q_EMIT shareErrorSignal();
    updateJob();
    break;
  case EthereumPoolError::UnauthorizedWorker:
    Logger::critical(d->m_logChannel, QString("Failed to login! (msg=\"%1\"), disconnect and sleep...").arg(_message));
    stop();
    d->m_loginTimer.start();
    break;
  default:
    Logger::critical(d->m_logChannel, QString("Error! (msg=\"%1\" code=\"%2\" method=\"%3\"), disconnect and sleep...").
      arg(_message).
      arg(_code).
      arg(_request.value("method").toString()));
    stop();
    MiningJob job;
    Q_EMIT newJobSignal(job);
    stop();
    d->m_loginTimer.start();
    break;
  }
}

void PoolClient::socketError(QAbstractSocket::SocketError _error, const QString &_message) {
  Q_D(PoolClient);
  reset();
  Logger::critical(d->m_logChannel, QString("PoolClient: ERROR: %1").arg(_message));
  d->m_currentWorkerId = QString();
  Q_EMIT stateChangedSignal(PoolState::Error);
}

void PoolClient::updateProxy() {
  setProxy(QNetworkProxy::applicationProxy());
}

}
