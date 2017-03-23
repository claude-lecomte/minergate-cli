#include "miner-impl/ethereum/ethereumminerimpl.h"
#include "miner-impl/ethereum/poolthread.h"
#include "miner-impl/abstractminerimpl_p.h"

#include "miner-abstract/shareevent.h"

#include "ethutils/dagcontroller.h"

#include "utils/settings.h"
#include "utils/logger.h"
#include "utils/environment.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QAtomicPointer>

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate {

const quint32 DAG_LOADING_TIME(2000);

class EthereumMinerImplPrivate: public AbstractMinerImplPrivate {
public:
  QScopedPointer<PoolThread> m_poolThread;

#ifndef Q_OS_MAC
  EthereumMinerImplPrivate(const QString &_coinKey, bool _fee, QMap<ImplType, GPUManagerPtr> _gpuManagers):
    AbstractMinerImplPrivate(CPUWorkerType::Ethereum, _coinKey, _fee, _gpuManagers) {
#else
  EthereumMinerImplPrivate(const QString &_coinKey, bool _fee):
    AbstractMinerImplPrivate(CPUWorkerType::Ethereum, _coinKey, _fee) {
#endif
    m_startNonceIsRandom = true;
    m_isReady = false;
  }
};

#ifndef Q_OS_MAC
  EthereumMinerImpl::EthereumMinerImpl(const QUrl &_pool_url, const QString &_worker, const QString &_coinKey, bool _fee,
                                     const QMap<ImplType, GPUManagerPtr> &_gpuManagers):
    AbstractMinerImpl (*new EthereumMinerImplPrivate(_coinKey, _fee, _gpuManagers)) {
#else
  EthereumMinerImpl::EthereumMinerImpl(const QUrl &_pool_url, const QString &_worker, const QString &_coinKey, bool _fee):
    AbstractMinerImpl (*new EthereumMinerImplPrivate(_coinKey, _fee)) {
#endif

  Q_D(EthereumMinerImpl);

  connect(&DagController::instance(), &DagController::progressUpdated, this, &AbstractMinerImpl::preparingProgress, Qt::QueuedConnection);
  connect(this, &EthereumMinerImpl::dagThreadFinished, this, &EthereumMinerImpl::dagThreadFinishedHandler, Qt::QueuedConnection);

  d->m_poolThread.reset(new PoolThread(_pool_url, _worker, QString::null, _coinKey));
  d->m_poolThread->moveToThread(d->m_poolThread.data());

  connect(d->m_poolThread.data(), &PoolThread::stateChangedSignal, this, &EthereumMinerImpl::poolClientStateChanged, Qt::QueuedConnection);
  connect(d->m_poolThread.data(), &PoolThread::newJobSignal, this, &EthereumMinerImpl::updateJob, Qt::QueuedConnection);
  connect(d->m_poolThread.data(), &PoolThread::shareSubmittedSignal, this, &EthereumMinerImpl::shareSubmittedSignal, Qt::QueuedConnection);
  connect(d->m_poolThread.data(), &PoolThread::shareErrorSignal, this, &EthereumMinerImpl::shareErrorSignal, Qt::QueuedConnection);
  connect(d->m_poolThread.data(), &PoolThread::jobDifficultyChangedSignal, this, &EthereumMinerImpl::jobDifficultyChangedSignal, Qt::QueuedConnection);

  d->m_poolThread->start(QThread::HighPriority);
}

EthereumMinerImpl::~EthereumMinerImpl() {
  Q_D(EthereumMinerImpl);
  if (d->m_poolThread) {
    d->m_poolThread->quit();
    d->m_poolThread->wait(10000);
  }
}

void EthereumMinerImpl::switchToPool(const QUrl &_pool_url) {
  Q_D(EthereumMinerImpl);
  QMetaObject::invokeMethod(d->m_poolThread.data(), "switchToPool", Qt::QueuedConnection, Q_ARG(QUrl, _pool_url));
}

void EthereumMinerImpl::setPool(const QUrl &_pool_url) {
  Q_D(EthereumMinerImpl);
  QMetaObject::invokeMethod(d->m_poolThread.data(), "setPool", Qt::QueuedConnection, Q_ARG(QUrl, _pool_url));
}

void EthereumMinerImpl::setWorker(const QString &_worker) {
  Q_D(EthereumMinerImpl);
  QMetaObject::invokeMethod(d->m_poolThread.data(), "setWorkerName", Qt::QueuedConnection, Q_ARG(QString, _worker));
}

void EthereumMinerImpl::poolStartListen() {
  Q_D(EthereumMinerImpl);
  QMetaObject::invokeMethod(d->m_poolThread.data(), "startListen", Qt::QueuedConnection);
}

void EthereumMinerImpl::poolStopListen() {
  Q_D(EthereumMinerImpl);
  QMetaObject::invokeMethod(d->m_poolThread.data(), "stopListen", Qt::QueuedConnection);
  d->m_isReady = false;
}

void EthereumMinerImpl::onShareEvent(ShareEvent *_event) {
  Q_D(EthereumMinerImpl);
  const QByteArray &jobId = _event->jobId();
  if (jobId == d->m_currentJob.job_id)
    QMetaObject::invokeMethod(d->m_poolThread.data(), "submitShare", Qt::QueuedConnection,
                              Q_ARG(QByteArray, jobId), Q_ARG(quint64, _event->nonce()),
                              Q_ARG(QByteArray, _event->hash()));
}

void EthereumMinerImpl::updateJob(const MiningJob &_job) {
  if (!Env::consoleApp() || Env::verbose())
    Logger::info("eth", "update job");
  Q_D(EthereumMinerImpl);
  if (_job.blob.isEmpty()) {
    d->m_isReady = false;
    Logger::debug("eth", "empty job");
    AbstractMinerImpl::updateJob(_job);
    return;
  }

  if (d->m_preparingInProgress) {
    Logger::debug("eth", "calculation already started");
    return;
  }

  DagController *dagCtl = &DagController::instance();
  if (dagCtl->dagProgress() != 100)
    return; // DAG is currently being prepared

  //try to load current DAG, if not exists - create it
  QThread *thread = new QThread;
  if (dagCtl->currentSeedhashData() != _job.blob || !dagCtl->dag()) {
    Logger::info(d->m_coinKey, "preparing unloaded DAG..." + QString(_job.blob.toHex()));
    d->m_preparingInProgress = true;
    QSharedPointer<QTimer> timer(new QTimer);
    connect(timer.data(), &QTimer::timeout, this, [this, timer]() {
      // send start of preparing
      timer->stop();
      Q_EMIT preparingProgress(0);
    });
    connect(thread, &QThread::started, [_job, dagCtl, thread, d] () {
      PreparingError error = PreparingError::NoError;
      if (dagCtl->prepareDag(_job.blob, d->m_preparingInProgress, error))
        Logger::debug(d->m_coinKey, "DAG ready");
      else {
        Logger::warning(d->m_coinKey, "DAG creating aborted");
        thread->setProperty("aborted", true);
        if (d->m_preparingInProgress) {
          thread->setProperty("error", QVariant::fromValue(error));
        }
      }
      thread->quit();
    });
    connect(thread, &QThread::finished, [_job, thread, timer, this] () {
      Q_EMIT dagThreadFinished(_job, thread, timer);
    });
    connect(qApp, &QCoreApplication::aboutToQuit, [d] () {
      d->m_preparingInProgress = false;
    });
    thread->start();
    timer->start(DAG_LOADING_TIME);
  } else {
    Logger::debug(d->m_coinKey, "DAG not changed");
    d->m_isReady = true;
    AbstractMinerImpl::updateJob(_job);
  }
}

void EthereumMinerImpl::dagThreadFinishedHandler(const MiningJob &_job, QThread *_thread, const QSharedPointer<QTimer> &_timer) {
  Q_D(EthereumMinerImpl);
  Q_ASSERT (_thread);
  Q_ASSERT(_timer);
  _timer->stop();
  d->m_preparingInProgress = false;
  if (Settings::instance().appQuitInProcess()) {
    _thread->deleteLater();
    return;
  }
  if (_thread->property("aborted").toBool()) {
    // handle error
    PreparingError error = _thread->property("error").value<PreparingError>();
    QString errMessage;
    switch (error) {
    case PreparingError::NoError: Q_EMIT preparingProgress(0); return;
    case PreparingError::NoEnoughSpace: errMessage = tr("Low disk space"); break;
    case PreparingError::NoMemory: errMessage = tr("No free memory"); break;
    default: errMessage = tr("Unknow error"); break;
    }
    Q_EMIT preparingError(errMessage);
    return;
  }
  Logger::info("eth", "new DAG prepared successfully: " + _job.blob.toHex());
  AbstractMinerImpl::updateJob(_job);
  d->m_isReady = true;
  // now calculate next DAG for unbroken mining when future epoch will started
  DagController::instance().prepareNext();
  _thread->deleteLater();
}

}

