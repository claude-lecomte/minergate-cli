#include <QCoreApplication>
#include <QStringList>
#include <QThread>
#include <QCommandLineParser>
#include <QTimer>
#include <QNetworkProxy>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined Q_OS_LINUX
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <signal.h>

#ifndef Q_OS_MAC
#ifndef NO_ETH
#include "gputester.h"
#endif
#endif

#include "utils/environment.h"
#include "utils/logger.h"
#include "utils/settings.h"

#ifndef Q_OS_MAC
#include "miner-core/gpumanagercontroller.h"

#include "miner-abstract/abstractgpuminer.h"

#include "cudaminer/cudadeviceprops.h"

#include "openclminer/cltypes.h"
#endif

#include "miner-core/minercorecommonprerequisites.h"
#include "miner-core/coinmanager.h"
#include "miner-core/coin.h"
#include "miner-core/serverapi.h"

#ifndef NO_ETH
#include "ethutils/dagcontroller.h"
#endif

#include "networkutils/networkenvironment.h"

using namespace MinerGate;

static void exitProc(int sig) {
  Logger::warning(QString::null, "Application is closing...");
#ifndef Q_OS_MAC
#ifndef NO_ETH
  GpuTester::stop();
#endif
#endif
  if (sig == SIGINT)
    qApp->quit();
}

static QPair<qreal, QString> normalizedHashRate(qreal _hashrate) {
  QStringList items;
  items << "H/s" << "kH/s" << "MH/s" << "GH/s";
  qreal tmp_hr(_hashrate);
  auto it = items.begin();
  while (tmp_hr > 1000) {
    tmp_hr /= 1000.;
    ++it;
  }

  return qMakePair(tmp_hr, *it);
}

int main(qint32 _argc, char *_argv[]) {
  qInstallMessageHandler(Logger::messageHandler);
  signal(SIGINT, exitProc);

#ifdef Q_OS_WIN
  SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
  QStringList paths = QCoreApplication::libraryPaths();
  paths.append(MinerGate::Env::appDir().absolutePath());
  QCoreApplication::setLibraryPaths(paths);
#elif defined Q_OS_LINUX
  //    setpriority(PRIO_PROCESS, getpid(), 19);
#endif

  QCoreApplication app(_argc, _argv);

  QCoreApplication::setApplicationName("minergate-cli");
  QCoreApplication::setApplicationVersion(Env::version());

  qputenv("CONSOLE_APP", "1");
#ifndef Q_OS_MAC
  qputenv("GPU_SINGLE_ALLOC_PERCENT", "98");

  struct GpuMinerDefine {
    QString deviceKey;
    CoinPtr miner;
    quint32 deviceOrderNo;

    GpuMinerDefine():
      deviceOrderNo(-1) {
    }

    GpuMinerDefine(const QString &_deviceKey, const CoinPtr &_miner, quint32 _deviceOrderNo):
      deviceKey(_deviceKey), miner(_miner), deviceOrderNo(_deviceOrderNo) {
    }

    GpuMinerDefine(const GpuMinerDefine &other):
      deviceKey(other.deviceKey),
      miner(other.miner),
      deviceOrderNo(other.deviceOrderNo) {
    }
  };
  QMap<QString, GpuMinerDefine> gpuMiners;
#endif

  QMap<QString, CoinPtr> cpuMiners;
  QString user;
  QNetworkProxy proxy;
  const qint32 MAXTHREADS(QThread::idealThreadCount());

  NetworkEnvironment::instance();
  QStringList args = QCoreApplication::arguments();
  args.removeFirst();
  if (args.isEmpty())
    args << "-h";

#ifndef Q_OS_MAC
  QMap<QString, QVariant> gpuDevices; //deviceKey / deviceProps
#endif

  // check preinit commands
  for (quint32 i = 0; i < args.count(); ++i) {
    QString arg = args.at(i);
    if(arg == "--help" || arg == "-h") {
      qDebug("%s", QString("Usage:\n\n"
#ifndef Q_OS_MAC
                           "  minergate-cli [options] <currency-settings> <gpu-settings> \n\n"
#else
                           "  minergate-cli [options] <currency-settings> \n\n"
#endif
                           "Options:\n"
                           "  -u,\t--user\taccount email from minergate.com\n"
                           "  -v,\t--version\tshow version\n"
#ifndef Q_OS_MAC
                           "  -l,\t--list-gpu\tshow list of available GPU devices\n"
#ifndef NO_ETH
                           "  -a,\t--adjust-gpu\tadjust all available GPU devices for best performance\n"
#endif
#endif
                           "  --proxy\t\tproxy server URL. Supports only socks protocols\n"
                           "\t\t\t(for example: socks://192.168.0.1:1080\n"
                           "  --verbose\t\tshow detailed logs\n\n"
                           "Currency settings:\n"
                           "  --currency <threads>\n"
                           "  currency\t\tpossible values: eth bcn xmr qcn xdn fcn mcn aeon dsh inf8.\n"
                           "\t\t\tFor merged mining: <mm_cc>+qcn <mm_cc>+dsh <mm_cc>+inf8,\n"
                           "\t\t\twhere <mm_cc> is fcn or mcn\n"
                           "  <threads>\t\tthreads count for specified currency\n\n"
#ifndef Q_OS_MAC
                           "Gpu settings:\n"
                           "  --gpu# <currency> [intensity <intensity>] [cuda-gridsize <cuda-gridsize>]\n"
                           "\t\t\t[cuda-worksize <cuda-worksize>] [cl-localsize <cl-localsize>] "
                           "[cl-globalsize <cl-globalsize>]\n"
                           "  gpu# \t\tyou can get this value from the list displayed\n"
                           "\t\t\tby command --list-gpu\n"
                           "  currency\t\tpossible values: eth bcn xmr qcn xdn fcn mcn aeon dsh inf8.\n"
                           "\t\t\tFor merged mining:  <mm_cc>+qcn <mm_cc>+dsh <mm_cc>+inf8,\n"
                           "\t\t\twhere <mm_cc> is fcn or mcn\n"
                           "  intensity\t\tthis parameter is used for mining CryptoNote-based\n"
                           "\t\t\tcurrencies on CUDA devices.\n"
                           "\t\t\tValue's range: 1..4. Recommended and default value: 2.\n"
                           "\t\t\tWhen no value is set, the client uses default value\n"
                           "  cl-localsize\t\tOpenCL device parameter 'local work size'.\n"
                           "\t\t\tCan be power of 2 only\n"
                           "  cl-globalsize\tmultiplier for OpenCL device parameter 'global work size'\n"
                           "  cuda-gridsize\tCUDA device parameter 'grid size'\n"
                           "  cuda-worksize\tCUDA device parameter 'work size'\n"
#endif
                           "Examples:\n"
                           "  minergate-cli -u minergate@minergate.com --proxy socks://192.168.0.1:1080 --bcn 4\n"
#ifndef Q_OS_MAC
                           "  minergate-cli --user minergate@minergate.com --eth 6 --fcn+dsh 2 --gpu1 eth cl-localsize 64 cl-globalsize 4096 --gpu2 bcn intensity 2\n").toLocal8Bit().data());
#else
                           "  minergate-cli --user minergate@minergate.com --eth 6 --fcn+dsh 2\n").toLocal8Bit().data());
#endif
      return 0;
    } else if (arg == "--version" || arg == "-v") {
      qDebug("%s", QString("MinerGate v.%1").arg(Env::version()).toLocal8Bit().data());
      return 0;
#ifndef Q_OS_MAC
    } else if (arg == "--list-gpu" || arg == "-l") {
      // show GPU list
      gpuDevices = GPUManagerController::allDevices();
      if (!gpuDevices.isEmpty()) {
        qDebug() << "Available GPU devices:";
        int i = 0;
        Q_FOREACH (const QVariant &device, gpuDevices.values()) {
          QString name;
          if (const CudaDevicePropPtr &cudaParams = qvariant_cast<CudaDevicePropPtr>(device))
            name = QString(cudaParams->name);
          else if (const OCLDevicePtr &clParams = qvariant_cast<OCLDevicePtr>(device))
            name = QString(clParams->deviceName);
          qDebug("%s", QString("gpu%1\t%2").arg(i + 1).arg(name).toLocal8Bit().data());
          i++;
        }
      } else
        Logger::info(QString::null, "No available GPU devices found.");
      return 0;
#endif
    } else if (arg == "--proxy") {
      QUrl proxy_url(QUrl::fromUserInput(args.value(++i)));
      if (proxy_url.isEmpty())
        Logger::critical(QString::null, "Incorrect proxy URL");
      else {
        if (!proxy_url.scheme().toLower().compare("socks")) {
          Logger::info(QString::null, QString("Using SOCKS proxy %1").arg(proxy_url.toString()));
          proxy.setType(QNetworkProxy::Socks5Proxy);
          proxy.setHostName(proxy_url.host());
          proxy.setPort(proxy_url.port());
          QNetworkProxy::setApplicationProxy(proxy);
        }
        else
          Logger::critical(QString::null, "Incorrect proxy protocol");
      }
      args.takeAt(i);
      args.takeAt(--i);
    } else if (arg == "--crashme") {
      int *p = nullptr;
      *p = 123;
    }
#ifndef Q_OS_MAC
#ifndef NO_ETH
    else if (arg == "-a" || arg == "--adjust-gpu") {
//      gpuDevices = GPUManagerController::allDevices();
      GpuTester tester;
      tester.run();
      return 0;
    }
#endif
#endif
  }

#ifndef Q_OS_MAC
  if (gpuDevices.isEmpty())
    gpuDevices = GPUManagerController::allDevices();
  if (!gpuDevices.isEmpty())
    qDebug() << QString("Count of found GPU-devices: %1").arg(gpuDevices.count());
#endif

  // initialize miners settings
  if (!MinerGate::ServerApi::instance().init())
    return 0;

#ifndef NO_ETH
  bool dagCreationInProcess = false;
  QObject::connect(&DagController::instance(), &DagController::progressUpdated, [&dagCreationInProcess](quint8 pc) {
    dagCreationInProcess = pc != 100;
    if (dagCreationInProcess)
      qDebug() << QString("DAG creation in the process... %1%").arg(pc);
  });
#endif

  for (quint32 i = 0; i < args.size(); ++i) {
    QString arg = args.at(i);
    if (arg == "--user" || arg == "-u") {
      user = args.value(++i);
      Settings::instance().writeParam("email", user);
    }
    else if (arg == "--verbose")
      qputenv("VERBOSE", "1");
#ifndef Q_OS_MAC
    else if (arg.startsWith("--gpu")) {
      bool ok;
      QString gpu = arg;
      quint32 deviceOrderNo = gpu.remove("--gpu").toUInt(&ok) - 1;
      if (!ok) {
        Logger::critical(QString::null, QString("Unknown GPU argument \"%1\".").arg(arg.remove("--")));
        return 2;
      }
      // check for valid device number
      if (deviceOrderNo >= gpuDevices.count()) {
        Logger::critical(QString::null, QString("Invalid GPU device number: %1.").arg(deviceOrderNo + 1));
        return 3;
      }

      const QString deviceKey = gpuDevices.keys().at(deviceOrderNo);
      QStringList coin = args.at(++i).split('+');
      QString ticker;
      QString mergedMining;
      switch (coin.size()) {
      case 2:
        mergedMining = coin.value(0);
        ticker = coin.value(1).toLower();
        break;
      case 1:
        ticker = coin.value(0).toLower();
        break;
      default:
        Logger::critical(QString::null, QString("Unknown currency \"%1\".").arg(arg));
        return 2;
      }

      const QStringList GPU_PARAM_LIST {"intensity", "cuda-gridsize", "cuda-worksize", "cl-localsize", "cl-globalsize"};
      const CoinPtr &miner = CoinManager::instance().minersByTicker().value(ticker);
      if (miner.isNull()) {
        Logger::critical(QString::null, QString("Unknown currency \"%1\".").arg(ticker));
        return 2;
      } else if (gpuMiners.contains(ticker)) {
        Logger::warning(QString::null, QString("Miner already exists. Ignoring."));
        // skip all arguments for this miner
        while(i + 1 < args.count()) {
          QString nextArg = args.at(++i);
          if (GPU_PARAM_LIST.contains(nextArg))
            continue;
          bool ok = false;
          nextArg.toUInt(&ok);
          if (ok)
            continue;
          if (nextArg.startsWith("--")) {
            --i;
            break;
          }
        };
        continue;
      }

      // internal loop for parsing gpu command argument
      QMap<int, QVariant> deviceParams;
      for (i = i + 1; i < args.count(); i++) {
        QString subArg = args.at(i);
        if (subArg.startsWith("--")) {
          // next param started
          --i;
          break;
        }

        if (!GPU_PARAM_LIST.contains(subArg)) {
          Logger::critical(QString::null, QString("Unknown argument \"%1\".").arg(subArg));
          return 2;
        }
        bool ok;
        quint32 argValue = args.at(++i).toUInt(&ok);
        if (!ok) {
          Logger::critical(QString::null, QString("Invalid argument value \"%1\".").arg(args.at(i)));
          return 2;
        }

        if (subArg == "intensity")
          deviceParams.insert((int)CudaDeviceParam::Intensity, argValue);
        else if (subArg == "cuda-gridsize")
          deviceParams.insert((int)CudaDeviceParam::GridSize, argValue);
        else if (subArg == "cuda-worksize")
          deviceParams.insert((int)CudaDeviceParam::BlockSize, argValue);
        else if (subArg == "cl-localsize")
          deviceParams.insert((int)OclDeviceParam::LocalWorkSize, argValue);
        else if (subArg == "cl-globalsize")
          deviceParams.insert((int)OclDeviceParam::GlobalWorkSize, argValue);
      }

      miner->setGpuParams(deviceParams, deviceKey);
      GpuMinerDefine minerDefine {deviceKey, miner, deviceOrderNo};
      gpuMiners.insert(ticker, minerDefine);
    }
#endif
    else if (arg.startsWith("--")) {
      // CPU mining defines for specific coin
      QStringList coin = arg.remove("--").split('+');
      QString ticker;
      QString mergedMining;
      switch (coin.size()) {
      case 2:
        mergedMining = coin.value(0);
        ticker = coin.value(1).toLower();
        break;
      case 1:
        ticker = coin.value(0).toLower();
        break;
      default:
        Logger::critical(QString::null, QString("Unknown currency \"%1\".").arg(arg));
        return 2;
      }

      const CoinPtr &miner = CoinManager::instance().minersByTicker().value(ticker);
      if (miner.isNull()) {
        Logger::critical(QString::null, QString("Unknown currency \"%1\".").arg(ticker));
        return 2;
      } else if (cpuMiners.contains(ticker)) {
        Logger::warning(QString::null, QString("Miner already exists. Ignoring."));
        bool ok;
        args.value(++i).toUInt(&ok);
        if (!ok) --i;
        continue;
      }

      bool ok;
      quint32 threads = args.value(++i).toUInt(&ok);
      if (!ok) {
        // next arg is not count of threads
        threads = MAXTHREADS;
        --i;
      }
      miner -> setMergedMining(mergedMining);
      if(threads)
        miner -> setCpuCores(threads);
      miner -> setCpuRunning((bool)threads);
      cpuMiners.insert(ticker, miner);
    }
  }

  if(user.isEmpty()) {
    Logger::critical(QString::null, QString("user argument is missing!!! Exiting.."));
    return 1;
  }

  if (cpuMiners.isEmpty()
    #ifndef Q_OS_MAC
      && gpuMiners.isEmpty()
    #endif
      ) {
    Logger::critical(QString::null, QString("Currency not specified!!! Exiting.."));
    return 1;
  }

  QTimer hr_timer;
  hr_timer.setInterval(10000);
  hr_timer.start();

  QStringList allCoins;
  for (auto it = cpuMiners.constBegin(); it != cpuMiners.constEnd(); ++it) {
    const QString &coin = it.key();
    const CoinPtr &miner = it.value();
    miner->startCpu();
    allCoins << coin;
  }

#ifndef Q_OS_MAC
  for (auto it = gpuMiners.constBegin(); it != gpuMiners.constEnd(); ++it) {
    const QString &coin = it.key();
    const GpuMinerDefine &minerDefine = it.value();
    minerDefine.miner->startGpu(minerDefine.deviceKey);
    minerDefine.miner->setGpuRunning(true, minerDefine.deviceKey);
    if (!allCoins.contains(coin))
      allCoins << coin;
  }
#endif

  Q_FOREACH (const QString &ticker, allCoins) {
#ifndef Q_OS_MAC
  #ifdef NO_ETH
    QObject::connect(&hr_timer, &QTimer::timeout, [ticker, cpuMiners, gpuMiners, gpuDevices]() {
  #else
    QObject::connect(&hr_timer, &QTimer::timeout, [ticker, cpuMiners, gpuMiners, gpuDevices, &dagCreationInProcess]() {
  #endif // NO_ETH
#else
    QObject::connect(&hr_timer, &QTimer::timeout, [ticker, cpuMiners, &dagCreationInProcess]() {
#endif

#ifndef NO_ETH
      if (ticker.toLower() == "eth" && dagCreationInProcess)
        return;
#endif
#ifndef Q_OS_MAC
      qreal gpuHashRate = 0u;
#endif
      qreal cpuHashRate = 0u;
      CoinPtr miner;
      if (cpuMiners.contains(ticker)) {
        miner = cpuMiners.value(ticker);
        cpuHashRate = miner->cpuHashRate().second;
      }

#ifndef Q_OS_MAC
      if (gpuMiners.contains(ticker)) {
        const GpuMinerDefine &minerDefine = gpuMiners.value(ticker);
        if (!miner)
          miner = minerDefine.miner;
        else
          Q_ASSERT(miner == minerDefine.miner);

        gpuHashRate = 0;
        {
          const QVariant &deviceProps = gpuDevices.value(minerDefine.deviceKey);
          Q_ASSERT(deviceProps.isValid());
          ImplType type;
          if (!qvariant_cast<CudaDevicePropPtr>(deviceProps).isNull())
            type = ImplType::CUDA;
          else if (!qvariant_cast<OCLDevicePtr>(deviceProps).isNull())
            type = ImplType::OpenCL;
          else
            Q_ASSERT(false);

          const GPUManagerPtr &manager = GPUManagerController::get(type);
          Q_ASSERT(manager);
          quint32 deviceNo = manager->deviceNo(minerDefine.deviceKey);
          const GPUCoinPtr &gpuMiner = manager->getMiner(deviceNo, miner->family());
          Q_ASSERT(gpuMiner);
          gpuHashRate = gpuMiner->hashrate();
        }

      }
      qreal allHashrate = cpuHashRate + gpuHashRate;
#else
      qreal allHashrate = cpuHashRate;
#endif
      auto hr = normalizedHashRate(allHashrate);
      const QString hashrate = QString("%1 %2").arg(hr.first, 0, 'f', 2).arg(hr.second);
      QString msg = QString("%1 hashrate: %2").
          arg(miner -> custom() ? (QString("%1:%2").arg(miner -> currentPool().host()).arg(miner -> currentPool().port())) : (miner -> mergedMining().isEmpty() ? "" : miner -> mergedMining().toUpper() + "+") + miner -> coinTicker().toUpper()).
          arg(hashrate);
      qDebug().noquote() << msg;
    });
  }


  return app.exec();
}
