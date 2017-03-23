#include "miner-impl/ethereum/ethereumworker.h"

#include "miner-abstract/abstractcpuminerworker_p.h"
#include "miner-abstract/shareevent.h"

#include "libethcore/EthashAux.h"

#include "libdevcore/FixedHash.h"

#include "libethash/ethash.h"
#include "libethash/internal.h"

#include "ethutils/dagcontroller.h"

#include "utils/logger.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QElapsedTimer>

namespace MinerGate {

class EthereumWorkerPrivate: public AbstractCPUMinerWorkerPrivate {
public:
  DagType m_currentDag;

  EthereumWorkerPrivate(volatile std::atomic<quint32> &_hash_counter, bool _fee, QObject* _parent) :
    AbstractCPUMinerWorkerPrivate(_hash_counter, _fee, _parent) {
  }

  ~EthereumWorkerPrivate(){}

  void prepareCalculation() Q_DECL_OVERRIDE {
    m_currentDag = DagController::instance().dag();
  }

  static dev::h256 arrayToH256(const QByteArray &ar) {
    Q_ASSERT(ar.size() == 32);
    return dev::h256(reinterpret_cast<const byte *>(ar.constData()), dev::h256::ConstructFromPointer);
  }

  static QByteArray h256ToArray(const dev::h256 &h) {
    QByteArray ret(reinterpret_cast<const char*>(&h), sizeof(h));
    Q_ASSERT(ret.size());
    return ret;
  }

  void search(Nonce startNonce, Nonce lastNonce, bool) Q_DECL_OVERRIDE {
    Q_ASSERT(m_currentDag);

    using namespace std;
    using namespace dev;
    using namespace dev::eth;

    const h256 headerHash = arrayToH256(m_currentJob.job_id);
    const h256 boundary = arrayToH256(m_currentJob.target);
    const ethash_h256_t &hHash = *(ethash_h256_t*)headerHash.data();

    for (Nonce nonce = startNonce; Q_LIKELY(nonce <= lastNonce); nonce++) {
      if (Q_UNLIKELY(m_state != MinerThreadState::Mining))
        break;
      try {
        const ethash_return_value &ethashReturn = ethash_full_compute(m_currentDag->full, hHash, nonce);
        const h256 &result = h256((uint8_t*)&ethashReturn.result, h256::ConstructFromPointer);
        if (Q_UNLIKELY(result <= boundary)) {
          const h256 mixHash((uint8_t*)&ethashReturn.mix_hash, h256::ConstructFromPointer);

          bool ret = ethash_quick_check_difficulty(
                (ethash_h256_t const*)headerHash.data(),
                nonce,
                (ethash_h256_t const*)mixHash.data(),
                (ethash_h256_t const*)boundary.data()
                );
          if (Q_UNLIKELY(!ret)) {
            Logger::debug("eth", "Fail on preVerify");
            continue;
          }

          const EthashProofOfWork::Result &evalResult = EthashAux::eval(DagController::instance().currentSeedhash(), headerHash, (dev::eth::Nonce)(u64)nonce);
          if (Q_LIKELY(evalResult.value < boundary && evalResult.mixHash == mixHash)) {
            m_result = h256ToArray(mixHash);
            m_nonce = nonce;
            QCoreApplication::postEvent(m_parent, new ShareEvent(m_currentJob.job_id, m_nonce, m_result, false), Qt::HighEventPriority);
          } else
            Logger::debug("eth", "The solution found, but is not verified for difficulty.");
        }
      }
      catch (...) {
        Logger::warning("ETH", "Error call ETH library function");
        continue;
      }
      m_hash_counter++;
    }
  }
};

EthereumWorker::EthereumWorker(volatile std::atomic<quint32> &_hashCounter, bool _fee, QObject* _parent):
  AbstractCPUMinerWorker(*new EthereumWorkerPrivate(_hashCounter, _fee, _parent)) {
}

EthereumWorker::~EthereumWorker() {
}

}
