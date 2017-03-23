#pragma once
#include <QString>

class QProcess;

namespace MinerGate
{

struct CPUInfo {
  quint8 cores = 0;
  quint32 cacheSize = 0;
  quint32 MHz = 0;
  QString name;
};

struct MemInfo {
  quint64 size;
  //QString name; // no good way for get a memory type name without admin privs.
};

class HardInfo
{
public:
  enum: quint64 {
    KB = 1024,
    MB = KB * 1024,
    GB = MB * 1024,
    TB = GB * 1024
  };

  // methods for get hard info
  static void getMemInfo(MemInfo &_info);
  static void getCPUInfo(CPUInfo &_info);
  static void getOSInfo(QString &info);

private:
  static void setProcessEnv(QProcess &process);
};

}
