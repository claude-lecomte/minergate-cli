#include "miner-abstract/noncectl.h"
#include <QAtomicInteger>
#include <QMap>
#include <QList>
#include <QMutexLocker>
#include <QtMath>
#include <random>

namespace MinerGate {

class NonceCtlPrivate {
public:
  mutable QAtomicInteger<Nonce> m_nonce = 0;
  QAtomicInt m_isInvalid = 0;

  typedef QPair<NonceCtl*, NonceCtl*> NonceCtlsForCoin;
  typedef QMap<QString, NonceCtlsForCoin> CtlsMap;
  static CtlsMap m_nonceCtlList;
  static QMutex m_instanceMutex;
};

NonceCtlPrivate::CtlsMap NonceCtlPrivate::m_nonceCtlList;
QMutex NonceCtlPrivate::m_instanceMutex;

NonceCtl *NonceCtl::instance(const QString &_coin, bool _fee) {
  Q_ASSERT(!_coin.isEmpty());
  QMutexLocker l(&NonceCtlPrivate::m_instanceMutex);
  if (NonceCtlPrivate::m_nonceCtlList.contains(_coin)) {
    return _fee?NonceCtlPrivate::m_nonceCtlList.value(_coin).second:NonceCtlPrivate::m_nonceCtlList.value(_coin).first;
  }
  else {
    NonceCtl *instance = new NonceCtl;
    NonceCtl *feeInstance = new NonceCtl;
    NonceCtlPrivate::m_nonceCtlList.insert(_coin, qMakePair(instance, feeInstance));
    return instance;
  }
}

Nonce NonceCtl::nextNonce(Nonce reserve) const {
  Q_ASSERT(Q_LIKELY(reserve > 0));
  auto newNonce =  d_ptr->m_nonce + reserve;
  if (Q_UNLIKELY(newNonce < d_ptr->m_nonce))
    newNonce = reserve; // check for 64-bit integer overflow
  d_ptr->m_nonce = newNonce;
  return d_ptr->m_nonce;
}

void NonceCtl::invalidate() {
  d_ptr->m_isInvalid = 1;
}

bool NonceCtl::isValid() const {
  return !d_ptr->m_isInvalid;
}

void NonceCtl::restart(bool randomStart) {
  if (randomStart) {
    std::random_device rd;
    d_ptr->m_nonce = std::uniform_int_distribution<Nonce>()(rd);
  } else
    d_ptr->m_nonce = 0;
  d_ptr->m_isInvalid = 0;
}

NonceCtl::NonceCtl():
  d_ptr(new NonceCtlPrivate) {
}

NonceCtl::~NonceCtl() {
  delete d_ptr;
}

}
