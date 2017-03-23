#include "utils/hardinfo.h"
#include <QThread>
#include <QProcess>

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>


#if defined(Q_OS_LINUX)
#include <sys/sysinfo.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <winbase.h>
#include <QSettings>
#elif defined(Q_OS_MAC)
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

const int BUFFERLEN(128);
#endif

#include "utils/logger.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate
{

void HardInfo::getMemInfo(MemInfo &_info) {
#if defined(Q_OS_LINUX)
    struct sysinfo info;
    sysinfo(&info);
    _info.size = info.totalram;
#elif defined(Q_OS_WIN32)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    _info.size = statex.ullTotalPhys;
#elif defined(Q_OS_MAC)
  int mib[2];
  int64_t physical_memory;
  size_t length;

  // Get the Physical memory size
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  length = sizeof(qint64);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);
  _info.size = physical_memory;
#else
#error Unrecognized platform
#endif
}

void HardInfo::getCPUInfo(CPUInfo &_info) {
  _info.cores = QThread::idealThreadCount();
#if defined(Q_OS_MAC)
  {
    quint64 freq = 0;
    size_t size = sizeof(freq);

    if (sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0) < 0)
      perror("sysctl");
    _info.MHz = freq / 1024.0 / 1024.0;
  }

  {
    quint64 cacheSize = 0;
    size_t size = sizeof(cacheSize);
    if (sysctlbyname("hw.l3cachesize", &cacheSize, &size, NULL, 0) < 0)
      perror("sysctl");
    _info.cacheSize = cacheSize;
  }

  {
    char buffer[BUFFERLEN];
    size_t bufferlen = BUFFERLEN;
    sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0);
    _info.name = buffer;
  }
#elif defined(Q_OS_WIN32)
  QSettings sets("HKEY_LOCAL_MACHINE\\Hardware\\Description\\System\\CentralProcessor\\0", QSettings::NativeFormat);
  _info.name = sets.value("ProcessorNameString").toString();

  {
    LARGE_INTEGER qwWait, qwStart, qwCurrent;
    QueryPerformanceCounter(&qwStart);
    QueryPerformanceFrequency(&qwWait);
    qwWait.QuadPart >>= 5;
    unsigned __int64 Start = __rdtsc();
    do
    {
      QueryPerformanceCounter(&qwCurrent);
    }while(qwCurrent.QuadPart - qwStart.QuadPart < qwWait.QuadPart);
    _info.MHz = ((__rdtsc() - Start) << 5) / 1000000.0;
  }

  // get cache info
  typedef BOOL (WINAPI *LPFN_GLPI)(
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
        PDWORD);

  LPFN_GLPI glpi;
  bool done = false;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = nullptr;
  DWORD returnLength = 0;
  DWORD byteOffset = 0;
  PCACHE_DESCRIPTOR Cache;

  glpi = (LPFN_GLPI) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");

  if (glpi) {
    while (!done) {
      DWORD rc = glpi(buffer, &returnLength);
      if (FALSE == rc) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
          if (buffer)
            free(buffer);

          buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                returnLength);

          if (nullptr == buffer) {
            Logger::critical(QString::null, "Error: Allocation failure");
            return;
          }
        }
        else {
          Logger::critical(QString::null, QString("Error %1").arg(GetLastError()));
          return;
        }
      }
      else
        done = TRUE;
    }
    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
      if (ptr->Relationship == RelationCache) {
        // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
        Cache = &ptr->Cache;
    if (Cache->Level == 3) {
      _info.cacheSize = Cache->Size;
      break;
    }
      }
      byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
      ptr++;
    }
  }
  else
    Logger::warning(QString::null, "GetLogicalProcessorInformation is not supported.");

#elif defined(Q_OS_LINUX)
  QProcess lscpu;
  setProcessEnv(lscpu);
  lscpu.start("lscpu", QIODevice::ReadOnly);
  if (lscpu.waitForFinished(2000)) {
    const QString &data = QString(lscpu.readAllStandardOutput());
    QStringList rows = data.split('\n');
    QString modelName;
    QString vendorId;
    Q_FOREACH (const QString &row, rows) {
      const QString key = row.section(':', 0,0).trimmed();
      const QString value = row.section(':', 1,1).trimmed();
      if (key.contains("L3 cache")) {
        const QString &unit = value.right(1);
        QString numStr = value.mid(0, value.count() - 1);
        int multiplier = 1;
        if (unit.contains("K"))
          multiplier = 1024;
        else if (unit.contains("M"))
          multiplier = 1024 * 1024;
        else
          numStr = value;
        bool ok;
        quint32 amount = numStr.toInt(&ok) * multiplier;
        if (!ok)
          continue;
        _info.cacheSize = amount;
      }
      else if (key == "CPU max MHz")
        _info.MHz = value.section(QRegExp("[,.]"), 0, 0).toInt();
      else if (key == "CPU MHz")
        _info.MHz = value.section(QRegExp("[,.]"), 0, 0).toInt();
      else if (key == "Model name")
        modelName = value;
      else if (key == "Vendor ID")
        vendorId = value;
    }
    _info.name = modelName.isEmpty()?vendorId:modelName;
  } else {
    Logger::critical(QString::null, "error call command: " + lscpu.errorString());
  }
#else
#error Unrecognized platform
#endif
}

void HardInfo::getOSInfo(QString &info) {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
  QProcess uname;
  setProcessEnv(uname);
  uname.start("uname -a", QIODevice::ReadOnly);
  if (uname.waitForFinished(2000))
    info = QString(uname.readAllStandardOutput());
#elif defined(Q_OS_WIN32)
  QProcess wmic;
  wmic.start("wmic os get Caption,CSDVersion /value", QIODevice::ReadOnly);
  if (wmic.waitForFinished(2000))
    info = QString(wmic.readAllStandardOutput()).remove("\r\n").trimmed();
#else
#error Unrecognized platform
#endif
}

void HardInfo::setProcessEnv(QProcess &process) {
  QStringList env = QProcess::systemEnvironment();
  for (int i = 0; i < env.count(); i++) {
    const QString &str = env.at(i);
    if (str.contains("LANG=")) {
      env.removeAt(i);
      break;
    }
  }

  env << "LANG=\"en_US\"";
  process.setEnvironment(env);
}

}
