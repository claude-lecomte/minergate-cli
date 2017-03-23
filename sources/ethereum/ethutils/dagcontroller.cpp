#include "dagcontroller.h"

#include "ethutils/testdata.h"

#include "libdevcore/SHA3.h"
#include "libdevcore/Exceptions.h"

#include "libethash/io.h"
#include "libethash/internal.h"
#include "libethash/mmap.h"

#include "miner-abstract/minerabstractcommonprerequisites.h"

#include "utils/logger.h"
#include "utils/settings.h"

#include <QMutex>
#include <QByteArray>
#include <QThread>
#include <atomic>
#include <QAtomicInteger>
#include <QFile>
#include <QDir>
#include <QStorageInfo>

#include <stdio.h>


#ifdef QT_DEBUG
#include  <QDebug>
#endif

namespace MinerGate {

  const QStringList DAG_FILTER { "full-R*" };

  class DagControllerPrivate {
  public:
    DagType m_dag;
    mutable QMutex m_mutex;
    QByteArray m_seedHashData;
    QAtomicInteger<quint16> m_progress = 100;
    QString m_dagDir;

    bool getDagDir(QDir &_dir) const {
      if (!m_dagDir.isEmpty()) {
        _dir.setPath(m_dagDir);
        return true;
      }

      char strbuf[256];
      if (!ethash_get_default_dirname(strbuf, 256))
        return false;
      _dir.setPath(QString(strbuf));
      return true;
    }

    static QString getFileName (const dev::h256 &seedHash) {
      char mutableName[DAG_MUTABLE_NAME_MAX_SIZE];
      const ethash_h256_t &sh = *reinterpret_cast<const ethash_h256_t *>(&seedHash);
      if (!ethash_io_mutable_name(ETHASH_REVISION, &sh, mutableName))
        return QString::null;
      return QString(mutableName);
    }

    DagControllerPrivate() {
      m_dagDir = Settings::instance().dagDir();
      dev::eth::EthashAux::get()->setDAGDirName(m_dagDir.toLocal8Bit().constData());
    }
  };

  DagController::~DagController() {
  }

  quint8 DagController::dagProgress() const {
    Q_D(const DagController);
    QMutexLocker l(&d->m_mutex);
    return d->m_progress;
  }

  dev::h256 DagController::currentSeedhash() const {
    Q_D(const DagController);
    QMutexLocker l(&d->m_mutex);
    return *reinterpret_cast<const dev::h256*>(d->m_seedHashData.constData());
  }

  QByteArray DagController::currentSeedhashData() const {
    Q_D(const DagController);
    QMutexLocker l(&d->m_mutex);
    return d->m_seedHashData;
  }

  quint64 DagController::currentDagSize() const {
    Q_D(const DagController);
    const quint64 blockNumber = dev::eth::EthashAux::number(currentSeedhash());
    return ethash_get_datasize(blockNumber);
  }

  void DagController::setDagDir(const QString &_dagDir) {
    Q_D(DagController);
    d->m_dagDir = _dagDir;
    dev::eth::EthashAux::get()->setDAGDirName(_dagDir.toLocal8Bit().constData());
  }

  void DagController::removeOldDags() const {
    Q_D(const DagController);
    return; // WARNING: temporary disable because eth and etc have some DAG-logic
    // remove old, unused DAGs
    QDir dir;
    if (!d->getDagDir(dir) || !dir.exists()) {
      Logger::debug(QString::null, "DAG directory does not exists or not available");
      return;
    }

    const dev::h256 &currentSeedHash = *reinterpret_cast<const dev::h256 *>(d->m_seedHashData.constData());
    const dev::h256 &nextSeedHash = dev::sha3(currentSeedHash);
    const dev::h256 &testSeedHash = dev::arrayToH256(QByteArray::fromHex(TEST_SEED_HASH.toLocal8Bit()));

    const QStringList validFileNames { d->getFileName(currentSeedHash),
          d->getFileName(nextSeedHash), d->getFileName(testSeedHash) };

    Q_FOREACH (const QFileInfo &fi, dir.entryInfoList(DAG_FILTER, QDir::Files)) {
      const QString &fileName = fi.fileName();
      if (!validFileNames.contains(fileName)) {
        const QString &removingFileName = fi.absoluteFilePath();
        if (QFile::remove(removingFileName))
          Logger::debug("eth", QString("removed old DAG-file: %1").arg(removingFileName));
        else
          Logger::critical("eth", QString("Can't delete old DAG-file: %1").arg(removingFileName));
      }
    }

  }

  DagController::DagController():
    QObject(),
    d_ptr(new DagControllerPrivate) {
  }

  bool DagController::prepareDag(const QByteArray &_seedHashData, volatile std::atomic<bool> &_inProgress, PreparingError &_error) {
    Q_D(DagController);
    _error = PreparingError::NoError;
    using namespace dev;
    using namespace dev::eth;
#ifdef QT_DEBUG
    Logger::debug("eth", QString("Preparing DAG with seedHash: %1").arg(QString(_seedHashData.toHex())));
#endif
    h256 seedHash;
    {
      QMutexLocker l(&d->m_mutex);
      d->m_dag.reset();
      d->m_seedHashData = _seedHashData;
      seedHash = *reinterpret_cast<const dev::h256*>(d->m_seedHashData.constData());
      d->m_progress = 0;
    }

    // check free space on disk
    {
      const QString &fileName = d->getFileName(seedHash);
      if (!QFileInfo::exists(fileName)) {
        // check available GPU memory size
        QDir dir;
        if (!d->getDagDir(dir)) {
          Logger::critical("eth", "Can't get ethash directory");
          _error = PreparingError::Unknown;
          d->m_progress = 100;
          return false;
        }
        dir.cdUp();
        QStorageInfo si(dir);
        qint64 free = si.bytesFree();
        if (free > 0 && free * 1.5 < currentDagSize()) {
          Logger::critical("eth", tr("No enough disk space (available %1 bytes only)").arg(si.bytesFree()));
          _error = PreparingError::NoEnoughSpace;
          d->m_progress = 100;
          return false;
        }
      }
    }

    auto callback = [&_inProgress, this, d](unsigned pc) -> int {
      if (!_inProgress || Settings::instance().appQuitInProcess()) {
        Logger::debug("ETH", "aborting DAG calculation...");
        return 1; // quit from calculation
      }
      else {
        {
          QMutexLocker l(&d->m_mutex);
          d->m_progress = pc;
        }
        Logger::debug("ETH", QString("%1% DAG preparing").arg(pc));
        Q_EMIT progressUpdated(pc);
        return 0;
      }
    };

    DagType dag;
    try {
      if (!(dag = EthashAux::full(seedHash, Settings::instance().dagInMemory(), true, callback))) {
        {
          QMutexLocker l(&d->m_mutex);
          d->m_progress = 100;
        }
        Logger::critical("eth", "Can't generate DAG");
        return false;
      }
    }
    catch (const DagError &de) {
      if (de.m_errNo == dfeNoMemory) {
        _error = PreparingError::NoMemory;
        Logger::critical("ETH", "[1] DAG preparing aborted with message: no memory");
      }
      else {
        _error = PreparingError::Unknown;
        Logger::critical("ETH", "[1] DAG preparing aborted with message: " + QString::fromStdString(de.what()));
      }
      d->m_progress = 100;
      return false;
    }
    catch (const ExternalFunctionFailure &e) {
      _error = PreparingError::Unknown;
      Logger::critical("ETH", "[1] DAG preparing aborted with message: " + QString::fromStdString(e.what()));
      d->m_progress = 100;
      return false;
    } 
	catch (...) {
      _error = PreparingError::Unknown;
      Logger::critical("ETH", "[2] DAG preparing aborted");
      d->m_progress = 100;
      return false;
    }

    dev::eth::EthashAux::FullAllocation *result = nullptr;
    {
      QMutexLocker l(&d->m_mutex);
      d->m_dag = dag;
      d->m_progress = 100;
      result = d->m_dag.get();
    }
    Q_EMIT progressUpdated(100);
    return result;
  }

  bool DagController::prepareTestDag(const QByteArray &_testSeedHash, volatile std::atomic<bool> &_inProgress, PreparingError &_error) {
    Q_D(DagController);
    // Create/load any available DAG for benchmark testing

    _error = PreparingError::NoError;
    using namespace dev;
    using namespace dev::eth;

    {
      QMutexLocker l(&d->m_mutex);
      d->m_dag.reset();
      d->m_progress = 0;
    }

    // try to load exists DAG
    QDir dir;
    if (!d->getDagDir(dir)) {
      Logger::critical("eth", "Can't get ethash directory");
      _error = PreparingError::Unknown;
      d->m_progress = 100;
      return false;
    }

    QByteArray testedSeedHash = _testSeedHash;

    // look the DAG dir for the any DAG-files
    const QFileInfoList &dags = dir.entryInfoList(DAG_FILTER, QDir::Files);
    if (!dags.isEmpty()) {
      // Selecting seedHash for DAG file name

      // get oldest DAG file
      const QString &dagFileName = dags.first().completeBaseName();
      // remove first 9 symbols
      const QString beginOfSeedHash = dagFileName.mid(9).toLower();
      // find suitable seedHash which begins on "beginOfSeedHash"
      h256 seedHash = *reinterpret_cast<const h256*>(testedSeedHash.constData());
      quint16 maxEpoch = 2048;
      bool found = false;
      for (quint16 i = 0; i < maxEpoch; i++) {
        const QString &seedHashStr = testedSeedHash.toHex().toLower();
        if (seedHashStr.startsWith(beginOfSeedHash)) {
          found = true;
          break;
        }
        seedHash = sha3(seedHash); // calc next seedHash
        testedSeedHash = h256ToArray(seedHash);
      }

      if (found)
        return prepareDag(testedSeedHash, _inProgress, _error);
    }

    // DAG no found - create test DAG bases on hardcoded seedHash
    return prepareDag(testedSeedHash, _inProgress, _error);
  }

  void DagController::prepareNext() {
    Q_D(const DagController);
    removeOldDags();
    //TODO: break on app exit
    using namespace dev;
    using namespace dev::eth;
    h256 seedHash;
    {
      QMutexLocker l(&d->m_mutex);
      seedHash = *reinterpret_cast<const h256*>(d->m_seedHashData.constData());
    }

    // async. start calculation
    QThread *thread = new QThread(this);
    connect(thread, &QThread::started, [d, seedHash] (){
      const h256 next = sha3(seedHash);
      const QByteArray hexNext = h256ToArray(next).toHex();
      Logger::info("eth", "now calc next seed " + hexNext);
      char strbuf[256];
      if (!d->m_dagDir.isEmpty())
        strcpy(strbuf, d->m_dagDir.toLocal8Bit().constData());
      else
        if (!ethash_get_default_dirname(strbuf, 256)) {
          return;
        }

      auto l = dev::eth::EthashAux::light(next);
      auto light = l->light;
      uint64_t full_size = ethash_get_datasize(light->block_number);
      ethash_h256_t seedHash = ethash_get_seedhash(light->block_number);
      int dagError = dfeNoError;
      auto result = ethash_full_new_internal(strbuf, seedHash, full_size, light, nullptr, &dagError, false);
      //TODO: process error
      if (result) {
        Logger::info("eth", "next DAG prepared " + hexNext);
        try {
#ifndef Q_OS_WIN
          munmap(result->data, sizeof(uint64_t));
#endif
          fclose(result->file);
          free(result);
        }
        catch(...) {
        }
      }
      else
        Logger::critical("eth", "can't prepare next DAG " + hexNext);
      //TODO: check it
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start(QThread::LowestPriority);
  }

  void DagController::clearDag() {
    //release DAG file
    Q_D(DagController);
    using namespace dev;
    using namespace dev::eth;

    {
      QMutexLocker l(&d->m_mutex);
      d->m_dag.reset();
    }
  }

  bool DagController::testDagExists() const {
    Q_D(const DagController);
    QDir dir;
    if (!d->getDagDir(dir) || !dir.exists())
      return false;

    return !dir.entryList(DAG_FILTER, QDir::Files).isEmpty();
  }

  DagType DagController::dag() const {
    Q_D(const DagController);
    QMutexLocker l(&d->m_mutex);
    return d->m_dag;
  }

  bool DagController::isValid() const {
    Q_D(const DagController);
    QMutexLocker l(&d->m_mutex);
    return !d->m_dag->data().empty();
  }

}
