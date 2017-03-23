#pragma once

#include "libethcore/EthashAux.h"
#include "libdevcore/FixedHash.h"

#include <atomic>
#include <QSharedPointer>

namespace MinerGate {

typedef dev::eth::EthashAux::FullType DagType;
enum class PreparingError {NoError, Unknown, NoEnoughSpace, NoMemory};

class DagControllerPrivate;
class DagController: public QObject {
  Q_OBJECT
  Q_DECLARE_PRIVATE(DagController)
  Q_DISABLE_COPY(DagController)
public:
  static DagController &instance() {
    static DagController m_instance;
    return m_instance;
  }

  bool prepareDag(const QByteArray &_seedHashData, volatile std::atomic<bool> &_inProgress, PreparingError &_error);
  bool prepareTestDag(const QByteArray &_testSeedHash, volatile std::atomic<bool> &_inProgress, PreparingError &_error);
  void prepareNext();
  void clearDag();
  bool testDagExists() const;
  DagType dag() const;
  bool isValid() const;
  ~DagController();
  quint8 dagProgress() const;
  dev::h256 currentSeedhash() const;
  QByteArray currentSeedhashData() const;
  quint64 currentDagSize() const;
  void setDagDir(const QString &_dagDir);
Q_SIGNALS:
  void progressUpdated(quint8 pc);
private:
  QSharedPointer<DagControllerPrivate> d_ptr;

  DagController();
  void removeOldDags() const;
};
}

Q_DECLARE_METATYPE(MinerGate::PreparingError)
