#pragma once

#include <QEvent>
#include "minerabstractcommonprerequisites.h"

namespace MinerGate {

enum class MinerEventType : quint32 {Share = QEvent::User};

class ShareEvent : public QEvent {
public:
  ShareEvent(const QByteArray &_job_id, Nonce _nonce, const QByteArray &_hash, bool _fee) : QEvent(static_cast<QEvent::Type>(MinerEventType::Share)),
    m_job_id(_job_id), m_nonce(_nonce), m_hash(_hash), m_fee(_fee) {
  }

  QByteArray jobId() const { return m_job_id; }
  Nonce nonce() const { return m_nonce; }
  QByteArray hash() const { return m_hash; }
  bool fee() const { return m_fee; }

private:
  const QByteArray m_job_id;
  const Nonce m_nonce;
  const QByteArray m_hash;
  const bool m_fee;
};

}
