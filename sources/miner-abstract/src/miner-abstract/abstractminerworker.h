#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QVariant>

namespace MinerGate {
enum class MinerThreadState: quint8;
class MiningJob;
class AbstractMinerWorkerPrivate;
class AbstractMinerWorker : public QObject {
  Q_OBJECT
  Q_DECLARE_PRIVATE(AbstractMinerWorker)
  Q_DISABLE_COPY(AbstractMinerWorker)
public:
  ~AbstractMinerWorker();
  MinerThreadState getState() const;
  virtual void start(const QString &_coinKey);
public Q_SLOTS:
  virtual void stop();
  virtual void updateJob(const MiningJob &_job);
  virtual void updateFeeJob(const MiningJob &_job);
Q_SIGNALS:
  void startSignal();
protected:
  QScopedPointer<AbstractMinerWorkerPrivate> d_ptr;

  AbstractMinerWorker(AbstractMinerWorkerPrivate &_d);
  virtual void setState(MinerThreadState _state);
protected:
  virtual void startMining() = 0;
};

}
