#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "miner-impl/cryptonote/cryptonoteworker.h"

#include "miner-abstract/miningjob.h"
#include "miner-abstract/minerabstractcommonprerequisites.h"
#include "miner-abstract/abstractcpuminerworker_p.h"
#include "miner-abstract/shareevent.h"

#include "hash.h"
#include "hashlite.h"

namespace MinerGate {

class CryptonoteWorkerPrivate: public AbstractCPUMinerWorkerPrivate {
public:
  crypto::cn_context context;
  crypto::hash hash;
  cryptolite::hash hashLite;
  bool m_lite;

  ~CryptonoteWorkerPrivate() {}

  CryptonoteWorkerPrivate(volatile std::atomic<quint32> &_hash_counter, bool _fee, QObject* _parent, bool _lite) :
    AbstractCPUMinerWorkerPrivate(_hash_counter, _fee, _parent),
    m_lite(_lite) {
  }

  void search(Nonce startNonce, Nonce lastNonce, bool feeMode) Q_DECL_OVERRIDE{
    const quint32 target = arrayToQuint32(m_currentJob.target);
    const quint32 last = lastNonce;
    if (m_lite) {
      for (quint32 nonce = startNonce; Q_LIKELY(nonce <= last); nonce++) {
        if (Q_UNLIKELY(m_state != MinerThreadState::Mining))
          break;
        char *data = const_cast<char *>(m_currentJob.blob.data());
        std::memcpy(&data[39], reinterpret_cast<char*>(&nonce), sizeof(nonce));
        std::memset(&hashLite, 0, sizeof(hashLite));
        cryptolite::cn_slow_hash(data, m_currentJob.blob.size(), hashLite, 1);
        if (Q_UNLIKELY(((quint32*)&hashLite)[7] < target)) {
          m_result = QByteArray((char*)&hashLite, sizeof(hashLite));
          m_nonce = nonce;
          QCoreApplication::postEvent(m_parent, new ShareEvent(m_currentJob.job_id, m_nonce, m_result, feeMode), Qt::HighEventPriority);
        }
        m_hash_counter++;
      }
    } else {
      for (quint32 nonce = startNonce; Q_LIKELY(nonce <= last); nonce++) {
        if (Q_UNLIKELY(m_state != MinerThreadState::Mining))
          break;
        char *data = const_cast<char *>(m_currentJob.blob.data());
        std::memcpy(&data[39], reinterpret_cast<char*>(&nonce), sizeof(nonce));
        std::memset(&hash, 0, sizeof(hash));
        crypto::cn_slow_hash(context, data, m_currentJob.blob.size(), hash);
        if (Q_UNLIKELY(((quint32*)&hash)[7] < target)) {
          m_result = QByteArray((char*)&hash, sizeof(hash));
          m_nonce = nonce;
          QCoreApplication::postEvent(m_parent, new ShareEvent(m_currentJob.job_id, m_nonce, m_result, feeMode), Qt::HighEventPriority);
        }
        m_hash_counter++;
      }
    }
  }
};

CryptonoteWorker::CryptonoteWorker(volatile std::atomic<quint32> &_hash_counter, bool _fee, QObject* _parent, bool _lite):
  AbstractCPUMinerWorker(*new CryptonoteWorkerPrivate(_hash_counter, _fee, _parent, _lite)) {
}

CryptonoteWorker::~CryptonoteWorker() {
}

}
