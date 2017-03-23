#include <QJsonObject>
#include <QJsonDocument>
#include <QQueue>
#include <QUuid>
#include <QTimer>
#include <QDataStream>
#include <QDateTime>
#include <QTcpSocket>

#include <cstring>

#include "miner-impl/stratumadapter.h"
#include "miner-impl/cryptonote/stratumclient.h"

#include "networkutils/downloadmanager.h"
#include "networkutils/networkenvironment.h"
//#include "networkutils/debugger.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/environment.h"

#include "miner-abstract/miningjob.h"

#include "qjsonrpc/qjsonrpcclient.h"

namespace MinerGate {

class StratumClientPrivate {
public:
  StratumClientPrivate(const QUrl &_pool_url, const QString &_worker, const QString &_password,
    const QString &_log_channel) : m_worker(_worker), m_password(_password), m_current_session_id(),
    m_current_job_id(), m_log_channel(_log_channel) {
  }

  QString m_worker;
  QString m_password;
  QString m_current_session_id;
  QString m_current_job_id;
  QTimer m_login_timer;
  QString m_log_channel;
  StratumAdapter m_adapter;
};

StratumClient::StratumClient(const QUrl &_pool_url, const QString &_worker, const QString &_password,
  const QString &_log_channel, QObject *_parent) : JsonRpc::JsonRpcClient(_pool_url),
  d_ptr(new StratumClientPrivate(_pool_url, _worker, _password, _log_channel)) {
  Q_D(StratumClient);
  d->m_login_timer.setInterval(3000);
  setAdapter(&d->m_adapter);

  connect(&d->m_adapter, &StratumAdapter::jsonrpc_response_login, this, &StratumClient::login);
  connect(&d->m_adapter, &StratumAdapter::jsonrpc_response_getjob, this, &StratumClient::getjob);
  connect(&d->m_adapter, &StratumAdapter::jsonrpc_notification_job, this, &StratumClient::getjob);
  connect(&d->m_adapter, &StratumAdapter::jsonrpc_response_submit, this, &StratumClient::submit);
  connect(&d->m_adapter, &JsonRpc::JsonRpcAdapter::jsonrpc_error,
    this, &StratumClient::error);

  connect(this, &JsonRpcClient::connected, this, &StratumClient::tryLogin);
  connect(this, &JsonRpcClient::socketErrorSignal, this, &StratumClient::socketError);
  connect(&NetworkEnvironment::instance(), SIGNAL(updateProxySignal()), SLOT(updateProxy()));
  connect(&d->m_login_timer, &QTimer::timeout, this, &StratumClient::tryLogin);
}

StratumClient::~StratumClient() {
}

void StratumClient::start() {
  Q_D(StratumClient);
  if (!d->m_current_session_id.isNull()) {
    return;
  }

  connectToHost();
}

void StratumClient::stop() {
  Q_D(StratumClient);
  d->m_login_timer.stop();
  disconnectFromHost();
  d->m_current_session_id = QString();
  Logger::info(d->m_log_channel, QString("Stratum client stopped"));
  Q_EMIT jobDifficultyChangedSignal(0);
}

void StratumClient::switchToPool(const QUrl &_pool_url) {
  if (_pool_url != url()) {
    stop();
    setUrl(_pool_url);
    start();
  }
}

void StratumClient::setPool(const QUrl &_pool_url) {
  setUrl(_pool_url);
}

void StratumClient::setWorkerName(const QString &_worker) {
  Q_D(StratumClient);
  d->m_worker = _worker;
}

void StratumClient::tryLogin() {
  Q_D(StratumClient);
  QVariantMap params_map;
  params_map.insert("agent", Env::userAgent());
  params_map.insert("login", d->m_worker);
  params_map.insert("pass", d->m_password);
  d->m_adapter.jsonrpc_request_login(params_map);
}

void StratumClient::updateJob() {
  Q_D(StratumClient);
  QVariantMap params_map;
  params_map.insert("id", d->m_current_session_id);
  d->m_adapter.jsonrpc_request_getjob(params_map);
}

void StratumClient::submitShare(const QByteArray &_jobId, quint32 _nonce, const QByteArray &_result) {
  Q_D(StratumClient);
  if (d->m_current_session_id.isEmpty()) {
    return;
  }

  QVariantMap params_map;
  params_map.insert("id", d->m_current_session_id);
  params_map.insert("job_id", QString(_jobId));
  QByteArray nonce_arr;
  QDataStream nonce_str(&nonce_arr, QIODevice::WriteOnly);
  nonce_str.setByteOrder(QDataStream::LittleEndian);
  nonce_str << _nonce;

  params_map.insert("nonce", QString::fromUtf8(nonce_arr.toHex()));
  params_map.insert("result", _result.toHex());

  d->m_adapter.jsonrpc_request_submit(params_map);

  Logger::debug(d->m_log_channel, QString("Submitting share!!! id=\"%1\" job_id=\"%2\" nonce=\"%3\" result=\"%4\"").
    arg(d->m_current_session_id).arg(QString(_jobId)).arg(QString::fromUtf8(nonce_arr.toHex())).arg(QString::fromUtf8(_result.toHex())));
}

void StratumClient::setNewJob(const QVariantMap &_job_map) {
  Q_D(StratumClient);
  QString job_id = _job_map.value("job_id").toString();
  if (job_id.isEmpty()) {
    Logger::debug(d->m_log_channel, "Job didn't change");
  } else {
    QByteArray blob = QByteArray::fromHex(_job_map.value("blob").toByteArray());
    QByteArray target_arr = QByteArray::fromHex(_job_map.value("target").toByteArray());

    quint32 target;
    quint64 job_diff;
    QDataStream target_str(target_arr);
    target_str.setByteOrder(QDataStream::LittleEndian);
    target_str >> target;

    job_diff = getDifficultyForTarget(target);

    Logger::info(d->m_log_channel, QString("New Job: job_id=\"%1\" blob=\"%2\" target=\"%3\"").arg(job_id).
      arg(QString::fromUtf8(blob.toHex())).arg(QString::fromUtf8(target_arr.toHex())));
    Logger::info(d->m_log_channel, QString("New difficulty: %1").arg(job_diff));
    d->m_current_job_id = job_id;
    MiningJob job(job_id.toLocal8Bit(), blob, quint32ToArray(target));

    reset(false);

    Q_EMIT newJobSignal(job);
    Q_EMIT jobDifficultyChangedSignal(job_diff);
  }
}

void StratumClient::login(QVariantMap _params) {
  Q_D(StratumClient);
  Q_EMIT stateChangedSignal(PoolState::Connected);
  d->m_login_timer.stop();
  if (_params.value("status").toString().compare("OK")) {
    Logger::critical(d->m_log_channel, QString("StratumClient: Login ERROR"));
    return;
  }

  Q_EMIT stateChangedSignal(PoolState::Connected);
  d->m_current_session_id = _params.value("id", QVariant()).toString();
  Logger::info(d->m_log_channel, QString("Successfully connected to pool: %1. session_id=\"%2\"").
    arg(url().toString()).arg(d->m_current_session_id));
  setNewJob(_params.value("job").toMap());
}

void StratumClient::getjob(QVariantMap _params) {
  setNewJob(_params);
}

void StratumClient::submit(QVariantMap _params) {
  Q_D(StratumClient);
  if (_params.value("status").toString().compare("OK")) {
    return;
  }

  if (Env::consoleApp() && Env::verbose()) {
    Logger::info(d->m_log_channel, QString("Share submitted successfully!!!"));
  } else {
    Logger::debug(d->m_log_channel, QString("Share submitted successfully!!!"));
  }

  Q_EMIT shareSubmittedSignal(QDateTime::currentDateTime());
}

void StratumClient::job(QVariantMap _params) {
  Logger::debug(QString(), "New JOB notification");
  setNewJob(_params);
}

void StratumClient::error(qint32 _code, QString _message, QByteArray _data, QVariantMap _request) {
  Q_D(StratumClient);
  switch (static_cast<CryptonotePoolError>(_code)) {
  case CryptonotePoolError::NoError:
    return;
  case CryptonotePoolError::InvalidShareData:
  case CryptonotePoolError::InvalidJobId:
  case CryptonotePoolError::UnknownJobShare:
  case CryptonotePoolError::SlightlyStaleShare:
  case CryptonotePoolError::ReallyStaleShare:
  case CryptonotePoolError::DuplicateShare:
  case CryptonotePoolError::InvalidShare:
    Logger::critical(d->m_log_channel, QString("Failed to submit share! (msg=\"%1\" id=\"%2\" job_id=\"%3\" nonce=\"%4\" result=\"%5\"), updating job...").
      arg(_message).
      arg(_request.value("params").toMap().value("id").toString()).
      arg(_request.value("params").toMap().value("job_id").toString()).
      arg(_request.value("params").toMap().value("nonce").toString()).
      arg(_request.value("params").toMap().value("result").toString()));

    reset(false);
    Q_EMIT shareErrorSignal();
    updateJob();
//    if (static_cast<PoolError>(_code) == PoolError::InvalidShare) {
//      QJsonObject debug_obj;
//      debug_obj.insert("timestamp", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"));
//      debug_obj.insert("share", QJsonDocument::fromVariant(_request).object());
//      Debugger::instance().debug(QJsonDocument(debug_obj).toJson());
//    }

    return;
  case CryptonotePoolError::CanGetJob:
    Logger::critical(d->m_log_channel, QString("Failed to login! (msg=\"%1\"), disconnect and sleep...").arg(_message));
    stop();
    d->m_login_timer.start();
    return;
  default:
    Logger::critical(d->m_log_channel, QString("Error! (msg=\"%1\" code=\"%2\" method=\"%3\"), disconnect and sleep...").
      arg(_message).
      arg(_code).
      arg(_request.value("method").toString()));
    stop();
    MiningJob job;
    Q_EMIT newJobSignal(job);
    stop();
    d->m_login_timer.start();
    return;
  }
}

void StratumClient::socketError(QAbstractSocket::SocketError _error, const QString &_message) {
  Q_D(StratumClient);
  reset();
  Logger::critical(d->m_log_channel, QString("PoolClient: ERROR: %1").arg(_message));
  d->m_current_session_id = QString();
  Q_EMIT stateChangedSignal(PoolState::Error);
}

void StratumClient::updateProxy() {
  setProxy(QNetworkProxy::applicationProxy());
}

}
