#pragma once

#include <QObject>

#include "minercorecommonprerequisites.h"
#ifndef Q_OS_MAC
#include "miner-abstract/abstractgpumanager.h"
#endif
#include "utils/familyinfo.h"

namespace MinerGate {

const QString PAR_MINER("miner");
enum class MinerImplState: quint8;
class AbstractMinerEnginePrivate;
class _MinerCoreCommonExport AbstractMinerEngine : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(AbstractMinerEngine)
  Q_DECLARE_PRIVATE(AbstractMinerEngine)

public:
  virtual ~AbstractMinerEngine();

  HashFamily family() const;
  QString coinKey() const;
  QString coinTicker() const;
  QString coinName() const;
  quint8 accuracy() const;
  bool isReady() const;

  void startCpu(quint32 _cpu_cores = 0);
  void stopCpu(bool _sync);
  void restart();
  void setCpuCores(quint32 _cpu_cores);
  QPair<qreal, qreal> cpuHashrate() const;
#ifndef Q_OS_MAC
  void startGpu(const QStringList &_deviceKeys);
  void stopGpu(bool _sync, const QString &_deviceKey);
  void setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey);
  void setGpuIntensity(quint8 _intensity, const QString &_deviceKey);
  QPair<qreal, qreal> gpuHashrate() const;
#endif

protected:
  AbstractMinerEngine(HashFamily _family, const QString &_coin_key, const QString &_coinTicker,
    const QString &_coinName, quint8 _accuracy);

  virtual void startCpuImpl(quint32 _cpu_cores);
  virtual void stopCpuImpl(bool _sync);
  virtual void restartImpl();
  virtual void setCpuCoresImpl(quint32 _cpuCores);
  virtual QPair<qreal, qreal> cpuHashrateImpl() const;
#ifndef Q_OS_MAC
  virtual void startGpuImpl(const QStringList &_freeDevicesKeys);
  virtual void stopGpuImpl(bool _sync, const QString &_deviceKey);
  virtual void setGpuParamsImpl(const QMap<int, QVariant> &_params, const QString &_deviceKey);
  virtual QPair<qreal, qreal> gpuHashrateImpl() const;
#endif

  virtual MinerImplPtr createMinerImpl(const QUrl &_pool, const QUrl &_devPool, const QString &_worker,
                                       const QString &_coinKey, bool _fee
#ifndef Q_OS_MAC
                                       ,const QMap<ImplType, GPUManagerPtr> &_gpuManagers
#endif
                                       ) const = 0;

  QVariantMap &minerParams() const;
  CoinPtr getMiner() const;
private Q_SLOTS:
  void cpuStateChanged(MinerImplState _state);
  void gpuStateChanged(MinerImplState _state, const QString &_deviceKey);
  void shareSubmit(const QDateTime &_shareTime);
  void shareError();
  void jobDifficulty(quint64 _jobDifficulty);
  void defFeePoolChanged(const QUrl &_poolUrl);
  void preparingProgressChanged(quint8 pc);
  void preparingError(const QString &_message) const;
  void lowGpuMemory() const;
private:
  QScopedPointer<AbstractMinerEnginePrivate> d_ptr;

  MinerImplPtr getMinerImpl(bool create = true) const;
};
}
