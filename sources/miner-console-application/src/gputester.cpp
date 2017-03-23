#include "gputester.h"

#include <QVariant>
#include <QProcess>
#include <QCoreApplication>

#include "miner-core/gpumanagercontroller.h"
#include "utils/logger.h"

#include <cudaminer/cudadeviceprops.h>

#include <openclminer/cltypes.h>

#include "utils/utilscommonprerequisites.h"

#include "ethutils/testdata.h"
#include "ethutils/dagcontroller.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif


namespace MinerGate {

const quint32 PROCESS_TIMEOUT = 2 * 60 * 1000; // 30 minutes
const QList<quint16> ETH_AVAILABLE_BLOCK_SIZE = {32u, 64u, 128u, 256u};

class GpuTesterPrivate {
public:
  static bool m_stop;
  static bool m_started;
};

bool GpuTesterPrivate::m_stop = false;
bool GpuTesterPrivate::m_started = false;

GpuTester::GpuTester():
  d_ptr(new GpuTesterPrivate) {
  Logger::disableLogFiles();
}

GpuTester::~GpuTester() {
  Logger::disableLogFiles(false);
}

void GpuTester::run() {
  Q_D(GpuTester);
  GpuTesterPrivate::m_started = true;
  // 1. for each device
  // 2. for each hash family
  const QMap<QString, QVariant> &gpuDevices = GPUManagerController::allDevices();
  if (gpuDevices.isEmpty()) {
    qDebug("%s", "No available GPU devices found.");
    return;
  }
  int deviceNo = 0;
  const QString appString("./gpu-tester");

  Q_FOREACH (const QVariant &device, gpuDevices.values()) {
    QString name;
    ImplType type;
    if (const CudaDevicePropPtr &cudaParams = qvariant_cast<CudaDevicePropPtr>(device)) {
      type = ImplType::CUDA;
      name = QString(cudaParams->name);
    }
    else if (const OCLDevicePtr &clParams = qvariant_cast<OCLDevicePtr>(device)) {
      type = ImplType::OpenCL;
      name = QString(clParams->deviceName);
    }
    qDebug("%s", QString("%1. now testing GPU t%2").arg(deviceNo + 1).arg(name).toLocal8Bit().constData());

    {
      qDebug("%s", "preparing DAG...");
      DagController &dagCtl = DagController::instance();
      std::atomic<bool> inProgress(true);
      PreparingError error = PreparingError::NoError;
      const QByteArray &testSeedHash = QByteArray::fromHex(TEST_SEED_HASH.toLocal8Bit());
      if (!dagCtl.prepareTestDag(testSeedHash, inProgress, error)) {
        Logger::critical(QString::null, "Can't prepare test DAG");
        continue;
      }
      const DagType &dag = dagCtl.dag();
      if (!dag) {
        Logger::critical(QString::null, "DAG not ready");
        continue;
      }

      qDebug("%s", "DAG ready");
      dagCtl.clearDag(); // don't required more
    }

    QMap<quint32, QPair<quint32, quint32>> results;
    QString firstName, secondName;

    int passed = 0;
    if (type == ImplType::CUDA) {
      firstName = "--cuda-gridsize";
      secondName = "--cuda-blocksize";
      // test for eth only
      Q_FOREACH(quint16 blockSize, ETH_AVAILABLE_BLOCK_SIZE) {
        quint32 maxResult = 0;
        for (quint32 gridSize = 8; gridSize < 4096; gridSize *= 2) {
          qDebug("%s", QString("now test blockSize=%1 gridSize=%2").arg(blockSize).arg(gridSize).toLocal8Bit().constData());
          qApp->processEvents();
          if (GpuTesterPrivate::m_stop)
            return;
          QStringList args;
          args << "CUDA" << QString::number(deviceNo) << "3" << QString::number(gridSize) << QString::number(blockSize);
          QProcess p;
          p.start(appString, args);
          if (p.waitForFinished(PROCESS_TIMEOUT)) {
            const QString &data = QString(p.readAllStandardOutput());
            qDebug("%s", QString("result:" + data).toLocal8Bit().constData());
            bool ok;
            quint32 hashrate = data.toUInt(&ok);
            if (!hashrate || !ok) {
              Logger::critical(QString::null, data.toLocal8Bit().constData());
              break;
            }
            if (hashrate <= maxResult)
              break;
            maxResult = hashrate;
            if (!results.contains(hashrate))
              results.insert(hashrate, qMakePair(gridSize, blockSize));
            ++passed;
            qDebug("%s", QString("%1 %2 passed").arg(passed).arg(passed == 1?"test":"tests").toLocal8Bit().constData());
          } else
            Logger::critical(QString::null, "test failed with params: " + args.join(' '));
        }
      }
    } else if (type == ImplType::OpenCL) {
      firstName = "--cl-globalsize";
      secondName = "--cl-localsize";
      // test for eth only
      for (quint16 localSize = 8; localSize <= 256; localSize *= 2) {
        quint32 maxResult = 0;
        for (quint32 globalSize = 128; globalSize <= 4096; globalSize *= 2) {
          qDebug("%s", QString("now test localWorkSize=%1 globalWorkSize=%2").arg(localSize).arg(globalSize).toLocal8Bit().constData());
          qApp->processEvents();
          if (GpuTesterPrivate::m_stop)
            return;
          QStringList args;
          args << "OpenCL" << QString::number(deviceNo) << "3" << QString::number(localSize) << QString::number(globalSize);
          QProcess p;
          p.start(appString, args);
          if (p.waitForFinished(PROCESS_TIMEOUT)) {
            const QString &data = QString(p.readAllStandardOutput());
            qDebug("%s", QString("result:" + data).toLocal8Bit().constData());
            if (data.contains("error"))
              break; // end of current cycle
            bool ok;
            quint32 hashrate = data.toUInt(&ok);
            if (!hashrate || !ok) {
              Logger::critical(QString::null, data.toLocal8Bit().constData());
              break;
            }
            if (hashrate <= maxResult)
              break;
            maxResult = hashrate;
            if (!results.contains(hashrate))
              results.insert(hashrate, qMakePair(localSize, globalSize));
            ++passed;
            qDebug("%s", QString("%1 %2 passed").arg(passed).arg(passed == 1?"test":"tests").toLocal8Bit().constData());
          } else
            Logger::critical(QString::null, "test failed with params: " + args.join(' '));
        }
      }
    }

    if (results.isEmpty()) {
      Logger::warning(QString::null, "test failed: device is not supported");
      continue;
    }
    // display results
    QMapIterator<quint32, QPair<quint32, quint32>> it(results);
    it.toBack();
    int i = 0;
    QStringList slResults;
    while (it.hasPrevious() && i < 4) {
      it.previous();
      const QPair<quint32, quint32> &result = it.value();
      slResults.push_front(QString("%2=%3\t%4=%5")
                           .arg(firstName)
                           .arg(result.first)
                           .arg(secondName)
                           .arg(result.second));
      i++;
    }

    qDebug("%s",  "adjusting results:");
    for (int i = 0; i < slResults.count(); i++)
      qDebug("%s",  QString("%1. %2").arg(i + 1).arg(slResults.at(i)).toLocal8Bit().constData());
    deviceNo++;
  }
  GpuTesterPrivate::m_started = false;
}

void GpuTester::stop() {
  if (GpuTesterPrivate::m_started) {
    qDebug("%s", "test stopped");
    GpuTesterPrivate::m_stop = true;
    GpuTesterPrivate::m_started = false;
  }
}

}
