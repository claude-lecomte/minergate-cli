
#pragma once

#include <QObject>
#include <QAbstractSocket>
#include <QUrl>

#include <functional>
#include <stdint.h>

#include "miner-impl/minerimplcommonprerequisites.h"

#include "qjsonrpc/qjsonrpcclient.h"

namespace MinerGate {

class MiningJob;
struct Request;
typedef QSharedPointer<Request> RequestPtr;

class PoolClientPrivate;

class PoolClient : public JsonRpc::JsonRpcClient {
  Q_OBJECT
  Q_DISABLE_COPY(PoolClient)
  Q_DECLARE_PRIVATE(PoolClient)

public:
  PoolClient(const QUrl &_pool_url, const QString &_worker_name, const QString &_password,
    const QString &_log_channel, QObject *_parent = nullptr);
  ~PoolClient();
public Q_SLOTS:

  void start();
  void stop();

  void switchToPool(const QUrl &_pool_url);
  void setPool(const QUrl &_pool_url);
  void setWorkerName(const QString &_worker);
  void submitShare(const QByteArray &_headerHash, quint64 _nonce, const QByteArray &_mixHash);

private:
  QScopedPointer<PoolClientPrivate> d_ptr;

  void setNewJob(const QVariantList &_jobParams);
private Q_SLOTS:
  void tryLogin();
  void updateJob();
  void updateProxy();

  void login(QVariantList _params);
  void getjob(QVariantList _params);
  void submit(bool result);
  void job(QVariantList _params);
  void error(qint32 _code, QString _message, QByteArray _data, QVariantMap _request);
  void socketError(QAbstractSocket::SocketError _error, const QString &_message);

Q_SIGNALS:
  void stateChangedSignal(PoolState _state);
  void newJobSignal(const MiningJob &_job);
  void shareSubmittedSignal(const QDateTime &_time);
  void shareErrorSignal();
  void jobDifficultyChangedSignal(quint32 _diff);
};

}

Q_DECLARE_METATYPE(QUrl)
