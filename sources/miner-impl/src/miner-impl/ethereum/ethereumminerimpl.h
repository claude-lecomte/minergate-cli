#pragma once

#include "miner-impl/abstractminerimpl.h"
#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpumanager.h"
#endif

class QTimer;

namespace MinerGate {

class EthereumMinerImplPrivate;
class EthereumMinerImpl : public AbstractMinerImpl {
  Q_OBJECT
  Q_DECLARE_PRIVATE(EthereumMinerImpl)
  Q_DISABLE_COPY(EthereumMinerImpl)
public:
#ifndef Q_OS_MAC
  EthereumMinerImpl(const QUrl &_pool_url, const QString &_worker,
                    const QString &_coinKey, bool _fee, const QMap<ImplType, GPUManagerPtr> &_gpuManagers);
#else
  EthereumMinerImpl(const QUrl &_pool_url, const QString &_worker, const QString &_coinKey, bool _fee);
#endif
  ~EthereumMinerImpl();

  void switchToPool(const QUrl &_pool_url) Q_DECL_OVERRIDE;
  void setPool(const QUrl &_pool_url) Q_DECL_OVERRIDE;
  void setWorker(const QString &_worker) Q_DECL_OVERRIDE;

protected:
  void poolStartListen() Q_DECL_OVERRIDE;
  void poolStopListen() Q_DECL_OVERRIDE;
  void onShareEvent(ShareEvent *_event) Q_DECL_OVERRIDE;

  // AbstractMinerImpl interface
public Q_SLOTS:
  void updateJob(const MiningJob &_job) Q_DECL_OVERRIDE;

private Q_SLOTS:
  void dagThreadFinishedHandler(const MiningJob &_job, QThread *_thread, const QSharedPointer<QTimer> &_timer);

Q_SIGNALS:
  void dagThreadFinished(const MiningJob &_job, QThread *_thread, const QSharedPointer<QTimer> &_timer);
};

}
