#pragma once

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#ifdef Q_OS_WIN
#include <QSysInfo>
#elif defined Q_OS_LINUX
#include <QSettings>
#include <cpuid.h>

#else
#include <cpuid.h>
#endif

// Client name
#ifdef Q_OS_WIN64
#define CLIENT_NAME "MinerGateWin"
#define OS_FAMILY "Win64"
#elif defined Q_OS_WIN32
#define CLIENT_NAME "MinerGateWin32"
#define OS_FAMILY "Win32"
#elif defined Q_OS_LINUX
#ifdef RPM_BASED_LINUX
#define CLIENT_NAME "MinerGateRpm"
#else
#define CLIENT_NAME "MinerGateDeb"
#endif
#define OS_FAMILY "Linux"
#elif defined Q_OS_MAC
#define CLIENT_NAME "MinerGateMac"
#define OS_FAMILY "Mac"
#endif

namespace MinerGate {

class Env {
public:
  static inline bool consoleApp() {
    return qgetenv("CONSOLE_APP").toInt();
  }

  static inline bool verbose() {
    return qgetenv("VERBOSE").toInt();
  }

  static inline bool serviceApp() {
    return qgetenv("SERVICE_APP").toInt();
  }

  static inline QDir appDir() {
    return QDir(QCoreApplication::applicationDirPath());
  }

  static inline QDir dataDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
  }

  static inline QDir tempDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
  }

  static inline QDir docDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  }

  static inline QDir homeDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
  }

  static inline QDir logDir() {
    return QDir(dataDir().absoluteFilePath("log"));
  }

  static inline QString bin(const QString &_bin) {
    return QStandardPaths::findExecutable(_bin);
  }

  static inline QString version() {
    return QString("%1.%2").arg(QString(MAJOR_VERSION)).arg(QString(MINOR_VERSION));
  }

  static inline QPair<QString, QString> os() {
    QString os_family, os_version;
#ifdef Q_OS_WIN
    os_family = "Windows";
    switch(QSysInfo::windowsVersion()) {
    case QSysInfo::WV_XP:
      os_version = "XP";
      break;
    case QSysInfo::WV_2003:
      os_version = "Server 2003";
      break;
    case QSysInfo::WV_VISTA:
      os_version = "Vista";
      break;
    case QSysInfo::WV_WINDOWS7:
      os_version = "7";
      break;
    case QSysInfo::WV_WINDOWS8:
      os_version = "8";
      break;
    case QSysInfo::WV_WINDOWS8_1:
      os_version = "8.1";
      break;
    case QSysInfo::WV_WINDOWS10:
      os_version = "10";
      break;
    }
#elif defined Q_OS_MAC
    os_family = "Mac OS X";
    switch(QSysInfo::macVersion()) {
    case QSysInfo::MV_ELCAPITAN:
      os_version = "El Capitan";
      break;
    case QSysInfo::MV_YOSEMITE:
      os_version = "Yosemite";
      break;
    case QSysInfo::MV_MAVERICKS:
      os_version = "Mavericks";
      break;
    case QSysInfo::MV_MOUNTAINLION:
      os_version = "Mountain Lion";
      break;
    case QSysInfo::MV_LION:
      os_version = "Lion";
      break;
    case QSysInfo::MV_SNOWLEOPARD:
      os_version = "Snow Leopard";
      break;
    default:
      os_version = "< 10.6";
      break;
    }
#elif defined Q_OS_LINUX
    QSettings os("/etc/os-release", QSettings::IniFormat);
    os_family = os.value("NAME").value<QString>();
    os_version = os.value("VERSION_ID").value<QString>();
#endif
    return qMakePair(os_family, os_version);
  }

  static inline QString clientName() {
    return CLIENT_NAME;
  }

  static inline QString userAgent() {
    return QString("%1%2/%3").arg(CLIENT_NAME).arg(consoleApp() ? "-cli" : (serviceApp() ? "-service" : "")).arg(version());
  }

  static inline QList<quint32> cpuId() {
    static QList<quint32> cpu_id(getCpuId());
    return cpu_id;
  }

  static inline QList<quint32> getCpuId() {
    QList<quint32> res;
#if defined(_MSC_VER)
    qint32 cpuinfo[4];
    __cpuid(cpuinfo, 1);
    res << cpuinfo[0] << cpuinfo[1] << cpuinfo[2] << cpuinfo[3];

#else
    quint32 a, b, c, d;
    __cpuid(1, a, b, c, d);
    res << a << b << c << d;
#endif
    return res;
  }

  static inline QString statClientName() {
    QString res("MinerGateApp");
    if (consoleApp()) {
      res = "MinerGateCli";
    } else if (serviceApp()) {
      res = "MinerGateService";
    }

    return res;
  }

  static inline QPair<QString, QString> statOs() {
    QPair<QString, QString> res = os();
    res.first = OS_FAMILY;
#if defined Q_OS_LINUX
    QSettings os("/etc/os-release", QSettings::IniFormat);
    res.second = QString("%1 %2").arg(os.value("NAME").value<QString>()).arg(os.value("VERSION_ID").value<QString>());
#endif
    return res;
  }
};

}
