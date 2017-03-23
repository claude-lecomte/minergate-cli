#include "miner-impl/abstractminerimpl.h"
#include "miner-impl/abstractminerimpl_p.h"
#include "miner-impl/cpuminerworkerfactory.h"

#include "miner-abstract/noncectl.h"
#include "miner-abstract/shareevent.h"
#include "miner-abstract/abstractgpuminer.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"

#include "utils/settings.h"
#include "utils/familyinfo.h"

#include <utils/logger.h>

namespace MinerGate {

#define HASHRATE_TIMEOUT 500

AbstractMinerImpl::AbstractMinerImpl(AbstractMinerImplPrivate &_d):
  QObject(), d_ptr(&_d) {
  Q_D(AbstractMinerImpl);
  d->m_hrTimer.setInterval(HASHRATE_TIMEOUT);
  connect(&d->m_hrTimer, &QTimer::timeout, this, &AbstractMinerImpl::updateHashRate, Qt::QueuedConnection);

  int thread_count = QThread::idealThreadCount();
  if (thread_count <= 0)
    thread_count = 1;

  for (int i = 0; i < thread_count; ++i) {
    const QThreadPtr &thread = QThreadPtr(new QThread);
    const CPUMinerWorkerPtr &worker = CPUMinerWorkerFactory::create(d->m_type, d->m_cpuHashBuffer, d->m_fee, this, d->m_options);
    Q_ASSERT(worker);
    d->m_threads.append(thread);
    d->m_workers.append(worker);
    worker->moveToThread(thread.data());
    thread->start();
  }

#ifndef Q_OS_MAC
  Q_FOREACH (ImplType impl, d->m_gpuManagers.keys()) {
    const GPUManagerPtr &man = d->m_gpuManagers.value(impl);
    for (int i = 0; i < man->deviceCount(); i++) {
      HashFamily family = FamilyInfo::familyByCoin(d->m_coinKey);
      const GPUCoinPtr gpuMiner = man->getMiner(i, family);
      if (!gpuMiner)
        continue;
      GPUMinerWorkerPtr worker(new GPUMinerWorker(man, d->m_fee, this, gpuMiner->deviceNo(), family));
      if (worker->init()) {
        QThreadPtr thread(new QThread);
        worker->moveToThread(thread.data());
        thread->start();
        const QString &key = man->deviceKey(gpuMiner->deviceNo());
        d->m_gpuContextList.insert(impl, GPUCtxPtr(new GPUCtx {thread, worker, key}));
        connect(worker.data(), &GPUMinerWorker::finishedSignal, this, [worker, d, key, this]() {
          // check for low mem error
          if (!worker->gpuLowMemory()) {
            QTimer::singleShot(500, this, [d, key, this] () {
              if (Settings::instance().isGpuRunning(d->m_coinKey, key)) {
                stopGpu(key, false);
              }
            });
          } else
            Q_EMIT lowGpuMemory();
        });
      }
    }
  }
#endif
}

AbstractMinerImpl::~AbstractMinerImpl() {

}

qreal AbstractMinerImpl::cpuHashRate(quint32 _secs) {
  Q_D(AbstractMinerImpl);
  return qMax(0.0, d->m_cpuHrCounter.getHashrate(_secs));
}

#ifndef Q_OS_MAC
qreal AbstractMinerImpl::gpuHashRate(quint32 _secs) {
  Q_D(AbstractMinerImpl);
  return qMax(0.0, d->m_gpuHrCounter.getHashrate(_secs));
}
#endif

void AbstractMinerImpl::updateHashRate() {
  Q_D(AbstractMinerImpl);
  d->m_cpuHrCounter.addHRItem(d->m_cpuHashBuffer);
  d->m_cpuHashBuffer = 0;

#ifndef Q_OS_MAC
  quint32 sumHashes = 0;
  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
    if (Settings::instance().isGpuRunning(d->m_coinKey, ctx->deviceKey))
      sumHashes += ctx->worker->takeHashCount();
  }
  if (sumHashes)
    d->m_gpuHrCounter.addHRItem(sumHashes);
#endif
}

quint32 AbstractMinerImpl::getRunningThreadsCount() const {
  Q_D(const AbstractMinerImpl);
  quint32 res = 0;
  Q_FOREACH (auto& worker, d->m_workers) {
    if (worker->getState() != MinerThreadState::Exit) {
      ++res;
    }
  }

  return res;
}

void AbstractMinerImpl::poolClientStateChanged(PoolState _state) {
  Q_D(AbstractMinerImpl);
  switch (_state) {
  case PoolState::Connected:
    if (getRunningThreadsCount() > 0) {
      Q_EMIT cpuStateChangedSignal(MinerImplState::Running);
    }

#ifndef Q_OS_MAC
    Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
      if (ctx->worker->getState() != MinerThreadState::Exit) {
        Q_EMIT gpuStateChangedSignal(MinerImplState::Running, ctx->deviceKey);
        break;
      }
    }
#endif

    break;
  case PoolState::Error:
    updateJob(MiningJob());
    if (getRunningThreadsCount() > 0) {
      Q_EMIT cpuStateChangedSignal(MinerImplState::Error);
    }

#ifndef Q_OS_MAC
    Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
      if (ctx->worker->getState() != MinerThreadState::Exit) {
        Q_EMIT gpuStateChangedSignal(MinerImplState::Error, ctx->deviceKey);
        break;
      }
    }
#endif

    break;
  }
}

void AbstractMinerImpl::startCpu(quint32 _thread_count) {
  Q_D(AbstractMinerImpl);
  Q_ASSERT(_thread_count <= d->m_workers.size());

  poolStartListen();

  for (quint32 i = 0; i < _thread_count; ++i) {
    if (d->m_threads.at(i)->isFinished())
      d->m_threads.at(i)->start();
    const CPUMinerWorkerPtr &worker = d->m_workers.at(i);
    worker->updateJob(d->m_currentJob);
    worker->updateFeeJob(d->m_feeJob);
    worker->start(d->m_coinKey);
  }

  d->m_hrTimer.start();
  Q_EMIT cpuStateChangedSignal(MinerImplState::Running);
}

void AbstractMinerImpl::stopCpu(bool _sync) {
  Q_D(AbstractMinerImpl);
  Q_FOREACH (const auto &worker, d->m_workers)
    worker->stop();

  if (_sync) {
    Q_FOREACH (const auto &thread, d->m_threads)
      thread->quit();

    Q_FOREACH (const auto &thread, d->m_threads)
      thread->wait();
  }

  Q_EMIT cpuStateChangedSignal(MinerImplState::Stopped);
#ifndef Q_OS_MAC
  // check the state of all gpuMiners
  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
    if (ctx->worker->getState() != MinerThreadState::Exit && ctx->thread->isRunning())
      return;
  }
#endif

  d->m_hrTimer.stop();
  poolStopListen();
  QMutexLocker lock(&d->m_jobMutex);
  d->m_currentJob = MiningJob();
  if (d->m_preparingInProgress)
    d->m_preparingInProgress = false; // stop preparing progress
}

bool AbstractMinerImpl::startGpu(const QStringList &_devicesKeys) {
#ifndef Q_OS_MAC
  Q_D(AbstractMinerImpl);
  bool started = false;
  Q_FOREACH (const GPUCtxPtr& ctx, d->m_gpuContextList)
    if (_devicesKeys.contains(ctx->deviceKey)) {
      if (ctx->worker->init() && ctx->worker->getState() != MinerThreadState::Mining) {
        if (!started) {
          started = true;
          poolStartListen();
          d->m_hrTimer.start();
        }

        if (ctx->thread->isFinished())
          ctx->thread->start();
        ctx->worker->updateJob(d->m_currentJob);
        ctx->worker->updateFeeJob(d->m_feeJob);
        ctx->worker->start(d->m_coinKey);
        Q_EMIT gpuStateChangedSignal(MinerImplState::Running, ctx->deviceKey);
      }
    }
  return started;
#else
  Q_UNUSED(_devicesKeys);
  return false;
#endif
}

void AbstractMinerImpl::stopGpu(const QString &_deviceKey, bool _sync) {
#ifndef Q_OS_MAC
  Q_D(AbstractMinerImpl);
  struct MinerWaiting {
    QThreadPtr thread;
    QString deviceKey;
  };
  typedef QSharedPointer<MinerWaiting> WaitingCoinPtr;
  QList<WaitingCoinPtr> threadsToWait;

  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
    if (_deviceKey.isEmpty() || ctx->deviceKey == _deviceKey) {
      if (ctx->worker->getState() != MinerThreadState::Exit) {
        ctx->worker->stop();
      }
      if (_sync) {
        ctx->thread->quit();
        WaitingCoinPtr ptr;
        ptr.reset(new MinerWaiting {ctx->thread, ctx->deviceKey});
        threadsToWait << ptr;
      } else
        Settings::instance().setGpuRunning(d->m_coinKey, ctx->deviceKey, false);
      Q_EMIT gpuStateChangedSignal(MinerImplState::Stopped, ctx->deviceKey);
    }
  }

  Q_FOREACH (const WaitingCoinPtr &ptr, threadsToWait) {
    ptr->thread->wait(2000);
    Settings::instance().setGpuRunning(d->m_coinKey, ptr->deviceKey, false);
  }

  bool gpuIsRunning = false;
  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList)
    if (ctx->worker->getState() != MinerThreadState::Exit && ctx->worker->getState() != MinerThreadState::Stopping) {
      gpuIsRunning = true;
      break;
    }

  if (!getRunningThreadsCount() && !gpuIsRunning) {
    d->m_hrTimer.stop();
    poolStopListen();
    if (d->m_preparingInProgress)
      d->m_preparingInProgress = false; // stop preparing progress
    QMutexLocker lock(&d->m_jobMutex);
    d->m_currentJob = MiningJob();
  }

#else
  Q_UNUSED(_deviceKey);
  Q_UNUSED(_sync);
#endif
}

void AbstractMinerImpl::setThreadCount(quint32 _thread_count) {
  Q_D(AbstractMinerImpl);
  for (int i = 0; i < d->m_workers.count(); ++i) {
    const CPUMinerWorkerPtr &worker = d->m_workers.at(i);
    if (i < _thread_count) {
      if (worker->getState() == MinerThreadState::Exit) {
        worker->updateJob(d->m_currentJob);
        worker->updateFeeJob(d->m_feeJob);
        worker->start(d->m_coinKey);
      }
    } else
      worker->stop();
  }
  Q_EMIT cpuStateChangedSignal(_thread_count == 0 ? MinerImplState::Stopped : MinerImplState::Running);
}


void AbstractMinerImpl::setGpuParams(const QMap<int, QVariant> &_params, const QString &_deviceKey) {
#ifndef Q_OS_MAC
  Q_D(AbstractMinerImpl);
  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList) {
    if (ctx->deviceKey == _deviceKey)
      ctx->worker->setGpuParams(_params);
  }
#else
  Q_UNUSED(_params);
  Q_UNUSED(_deviceKey);
#endif
}

void AbstractMinerImpl::updateJob(const MiningJob &_job) {
  Q_D(AbstractMinerImpl);
  d->m_currentJob = _job;
  NonceCtl *nonceCtl = NonceCtl::instance(d->m_coinKey, false);

  nonceCtl->invalidate();
  Q_FOREACH (auto& worker, d->m_workers) {
    if (worker->getState() != MinerThreadState::Exit) {
      worker->updateJob(d->m_currentJob);
    }
  }

#ifndef Q_OS_MAC
  Q_FOREACH (const GPUCtxPtr &ctx, d->m_gpuContextList)
    if (ctx->worker->getState() != MinerThreadState::Exit)
      ctx->worker->updateJob(d->m_currentJob);
#endif
  nonceCtl->restart(d->m_startNonceIsRandom);
}

void AbstractMinerImpl::updateFeeJob(const MiningJob &_job) {
  Q_D(AbstractMinerImpl);
  QMutexLocker lock(&d->m_fee_job_mutex);
  d->m_feeJob = _job;
}

bool AbstractMinerImpl::event(QEvent *_event) {
  Q_D(AbstractMinerImpl);
  switch (static_cast<MinerEventType>(_event->type())) {
  case MinerEventType::Share: {
    onShareEvent(static_cast<ShareEvent*>(_event));
    return true;
  }

  default:
    break;
  }

  return QObject::event(_event);
}

bool AbstractMinerImpl::isReady() const {
  Q_D(const AbstractMinerImpl);
  return d->m_isReady;
}

}
