
#pragma once

#include <QThread>
#include <QUuid>

#include <atomic>
#include <mutex>

#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpumanager.h"
#endif
#include "miner-impl/minerimplcommonprerequisites.h"
#include "miner-impl/abstractminerimpl.h"

namespace MinerGate {

class CryptonoteMinerImplPrivate;
class CryptonoteMinerImpl: public AbstractMinerImpl {
  Q_OBJECT
  Q_DECLARE_PRIVATE(CryptonoteMinerImpl)
  Q_DISABLE_COPY(CryptonoteMinerImpl)
public:
  CryptonoteMinerImpl(const QUrl &_pool_url, const QUrl &_dev_fee_pool_url, const QString &_worker,
    const QString &_coinKey, bool _fee,
#ifndef Q_OS_MAC
                      const QMap<ImplType, GPUManagerPtr> &_gpuManagers,
#endif
                      bool _lite);
  ~CryptonoteMinerImpl();

public:
  void switchToPool(const QUrl &_pool_url) Q_DECL_OVERRIDE;
  void switchToDevFeePool(const QUrl &_pool_url) Q_DECL_OVERRIDE;
  void setPool(const QUrl &_pool_url) Q_DECL_OVERRIDE;
  void setWorker(const QString &_worker) Q_DECL_OVERRIDE;

protected:
  void poolStartListen() Q_DECL_OVERRIDE;
  void poolStopListen() Q_DECL_OVERRIDE;
  void onShareEvent(ShareEvent* _event) Q_DECL_OVERRIDE;
};

}
