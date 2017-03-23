#pragma once

#include <QString>
#include <QMetaType>

namespace MinerGate {

class MiningJob {
public:
  MiningJob() : job_id(), blob(), target(0) {
  }

  MiningJob(const QByteArray &_job_id, const QByteArray &_blob, const QByteArray &_target) :
    job_id(_job_id), blob(_blob), target(_target) {
  }

  MiningJob &operator=(const MiningJob &_job) {
    job_id = _job.job_id;
    blob = _job.blob;
    target = _job.target;
    return *this;
  }

  QByteArray job_id; // headerHash for eth
  QByteArray blob; // seedHash for eth
  QByteArray target; // boundary for eth
};

inline QByteArray quint32ToArray(quint32 n) {
  return QByteArray(reinterpret_cast<const char*>(&n), sizeof(n));
}

inline quint32 arrayToQuint32(const QByteArray &ar) {
  return *reinterpret_cast<const quint32*>(ar.constData());
}

}

Q_DECLARE_METATYPE(MinerGate::MiningJob)
