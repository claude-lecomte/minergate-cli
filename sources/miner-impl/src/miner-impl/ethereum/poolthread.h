#pragma once

#include <QThread>

class QDateTime;

namespace MinerGate {

enum class PoolState : quint8;
class MiningJob;
class PoolThreadPrivate;
class PoolThread : public QThread {
  Q_OBJECT
  Q_DECLARE_PRIVATE(PoolThread)
  Q_DISABLE_COPY(PoolThread)
public:
  PoolThread(const QUrl &_pool_url, const QString &_worker_name, const QString &_password, const QString &_log_channel);
  ~PoolThread();
public Q_SLOTS:
  void startListen();
  void stopListen();

  void switchToPool(const QUrl &_pool_url);
  void setPool(const QUrl &_pool_url);
  void setWorkerName(const QString &_worker);
  void submitShare(const QByteArray &_headerHash, quint64 _nonce, const QByteArray &_mixHash);
protected:
  void run() Q_DECL_OVERRIDE;
private:
  QScopedPointer<PoolThreadPrivate> d_ptr;
Q_SIGNALS:
  void stateChangedSignal(PoolState _state);
  void newJobSignal(const MiningJob &_job);
  void shareSubmittedSignal(const QDateTime &_time);
  void shareErrorSignal();
  void jobDifficultyChangedSignal(quint32 _diff);
};

}
