#include "miner-abstract/abstractcpuminerworker_p.h"
#include "miner-abstract/noncectl.h"

#include "utils/logger.h"

namespace MinerGate {

const quint32 NONCE_INTERVAL = 1000000;

void AbstractCPUMinerWorkerPrivate::startMining() {
  m_state = MinerThreadState::UpdateJob;
  NonceCtl *nonceCtl = NonceCtl::instance(m_coinKey, false);
  NonceCtl *nonceFeeCtl = nullptr;
  Nonce lastNonce = 0;
  if (m_fee) {
    // job for developers if using not minergate pool
    nonceFeeCtl = NonceCtl::instance(m_coinKey, true);
  }
  Q_FOREVER {
    switch (static_cast<MinerThreadState>(m_state)) {
    case MinerThreadState::Exit:
      return;
    case MinerThreadState::UpdateJob: {
      {
        QMutexLocker lock(&m_jobMutex);
        m_currentJob = m_job;
      }

      if (m_state == MinerThreadState::Exit) {
        return;
      }

      if (!m_currentJob.job_id.isEmpty()) {
        m_state = MinerThreadState::Mining;
      } else {
        Logger::debug(QString::null, "CPU waits for a job...");
        QThread::sleep(1);
        continue;
      }
      prepareCalculation();
      break;
    }

    default:
      break;
    }

    bool fee;
    if (Q_UNLIKELY(m_fee && qrand() % 100 < 2)) {
      fee = true;
      // dev fee enable for others pools -  2% of CPU time.
      const MiningJob originJob = m_currentJob;
      {
        QMutexLocker lock(&m_feeJobMutex);
        m_currentJob = m_feeJob;
      }

      if (m_currentJob.job_id.isEmpty()) {
        m_currentJob = originJob;
        continue;
      }

      Q_ASSERT(nonceFeeCtl);
      lastNonce = nonceFeeCtl->nextNonce(NONCE_INTERVAL);
      Nonce startNonce = lastNonce - NONCE_INTERVAL;
      search(startNonce, lastNonce, true);
      m_currentJob = originJob;
    } else {
      fee = false;
      lastNonce = nonceCtl->nextNonce(NONCE_INTERVAL);
      Nonce startNonce = lastNonce - NONCE_INTERVAL;
      search(startNonce, lastNonce);
    }
  }
}

}
