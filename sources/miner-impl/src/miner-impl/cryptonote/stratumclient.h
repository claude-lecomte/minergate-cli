
#pragma once

#include <QObject>
#include <QAbstractSocket>

#include <functional>
#include <stdint.h>

#include "miner-impl/minerimplcommonprerequisites.h"

#include "qjsonrpc/qjsonrpcclient.h"

namespace MinerGate {

class MiningJob;
struct Request;
typedef QSharedPointer<Request> RequestPtr;

class StratumClientPrivate;

class StratumClient : public JsonRpc::JsonRpcClient {
  Q_OBJECT
  Q_DISABLE_COPY(StratumClient)
  Q_DECLARE_PRIVATE(StratumClient)

public:
  StratumClient(const QUrl &_pool_url, const QString &_worker_name, const QString &_password,
    const QString &_log_channel, QObject *_parent = nullptr);
  ~StratumClient();

  void start();
  void stop();

  void switchToPool(const QUrl &_pool_url);
  void setPool(const QUrl &_pool_url);
  void setWorkerName(const QString &_worker);
  void submitShare(const QByteArray &_job_id, quint32 _nonce, const QByteArray &_result);

public Q_SLOTS:
  void tryLogin();

private:
  QScopedPointer<StratumClientPrivate> d_ptr;

  void setNewJob(const QVariantMap &_job_map);

  static quint64 inline getDifficultyForTarget(quint32 _target) {
    if (!_target) {
      return 0x00000000ffffffff;
    }

    return 0x00000000ffffffff / static_cast<quint64>(_target);
  }

private Q_SLOTS:
  void updateJob();
  void updateProxy();

  void login(QVariantMap _params);
  void getjob(QVariantMap _params);
  void submit(QVariantMap _params);
  void job(QVariantMap _params);
  void error(qint32 _code, QString _message, QByteArray _data, QVariantMap _request);
  void socketError(QAbstractSocket::SocketError _error, const QString &_message);

Q_SIGNALS:
  void stateChangedSignal(PoolState _state);
  void newJobSignal(const MiningJob &_job);
  void shareSubmittedSignal(const QDateTime &_time);
  void shareErrorSignal();
  void jobDifficultyChangedSignal(quint64 _diff);
};

}
