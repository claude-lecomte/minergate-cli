#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QUrl>

namespace MinerGate {

enum class PoolState: quint8;
enum class MinerImplState: quint8;
class MiningJob;
class ShareEvent;
class AbstractMinerImplPrivate;
class AbstractMinerImpl: public QObject {
  Q_OBJECT
  Q_DECLARE_PRIVATE(AbstractMinerImpl)
public:
  virtual ~AbstractMinerImpl();
  virtual void switchToPool(const QUrl &_pool_url) = 0;
  virtual void switchToDevFeePool(const QUrl &_pool_url) {Q_UNUSED(_pool_url);}
  qreal cpuHashRate(quint32 _secs);
#ifndef Q_OS_MAC
  qreal gpuHashRate(quint32 _secs);
#endif
  virtual void setPool(const QUrl &_pool_url) = 0;
  virtual void setWorker(const QString &_worker) = 0;
  bool isReady() const;

public Q_SLOTS:
  void updateHashRate();
  void startCpu(quint32 _thread_count);
  void stopCpu(bool _sync);
  void stopGpu(const QString &_deviceKey, bool _sync);
  bool startGpu(const QStringList &_devicesKeys);
  void setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey);
  void setThreadCount(quint32 _thread_count);
  void updateFeeJob(const MiningJob &_job);
  void poolClientStateChanged(PoolState _state);
  virtual void updateJob(const MiningJob &_job);
protected:
  QScopedPointer<AbstractMinerImplPrivate> d_ptr;

  AbstractMinerImpl(AbstractMinerImplPrivate &_d);
  virtual void poolStartListen() = 0;
  virtual void poolStopListen() = 0;
  virtual void onShareEvent(ShareEvent* _event) = 0;
  bool event(QEvent *_event) Q_DECL_OVERRIDE;
private:
  quint32 getRunningThreadsCount() const;
Q_SIGNALS:
  void startedSignal();
  void stoppedSignal();
  void cpuStateChangedSignal(MinerImplState _state);
  void gpuStateChangedSignal(MinerImplState _state, const QString &_deviceKey);
  void shareSubmittedSignal(const QDateTime &_time);
  void shareErrorSignal();
  void jobDifficultyChangedSignal(quint64 _diff);
  void preparingProgress(quint8 pc);
  void preparingError(const QString &_message);
  void lowGpuMemory();
};

}
