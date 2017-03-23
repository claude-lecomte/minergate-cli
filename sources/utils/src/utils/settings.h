
#pragma once

#include <QVariant>

#include "utilscommonprerequisites.h"

namespace MinerGate {

class SettingsPrivate;
class Group;
enum class HashFamily: quint8;

class Settings {
  Q_DISABLE_COPY(Settings)
  Q_DECLARE_PRIVATE(Settings)
  friend class Group;
public:
  static inline Settings &instance() {
    static Settings inst;
    return inst;
  }

  QString language() const;
  QVariantMap minerList() const;
  QVariantMap customMinerList() const;

  bool startOnLoginEnabled() const;
  void setStartOnLoginEnabled(bool _on);

  bool addMiner(const QVariantMap &_params);
  bool addCustomMiner(const QVariantMap &_params);
  bool removeCustomMiner(const QString &_key);
  void setLanguage(const QString &_lang);

  void writeParam(const QString &_name, const QVariant &_value);
  QVariant readParam(const QString &_name, const QVariant &_default = QVariant()) const;
  void writeMinerParam(const QString &_coin_abbr, const QString &_name, bool _custom, const QVariant &_value);
  QVariant readMinerParam(const QString &_coin_abbr, const QString &_name, bool _custom, const QVariant &_default = QVariant()) const;

  quint8 gpuIntensity(const QString &_coin_abbr, const QString _deviceKey) const;
  void setGpuIntensity(const QString &_coin_abbr, const QString _deviceKey, quint8 value);

  bool isGpuRunning(const QString &_coin_abbr, const QString _deviceKey) const;
  void setGpuRunning(const QString &_coin_abbr, const QString _deviceKey, bool _value);

  QStringList childKeys(const QString &_section) const;

  bool wasStartingBenchmark() const;
  void setStartingBenchmark(bool was = true);

  void saveTestData(const QByteArray &data);
  QByteArray testData() const;
  void clearTestData();

  void saveAchievements(const QByteArray &_data);
  QByteArray achievements(quint32 &uiVer) const;
  void removeAchievementParam();
  bool tryToRestoreAchievements();

  void setSuppressAchievementNotify(bool suppress);
  bool suppressAchievementNotify() const;

  void sync();

  void setAppQuitInProcess(bool isQuit);
  bool appQuitInProcess() const;
  static QString formatDateTime(const QDateTime& dateTime, bool _withSeconds = false);
  static QString formatDate(const QDate &date);
  static QString formatTime(const QTime& time);

  // ETH-CUDA
  void ethCudaSetGridSize(quint32 _deviceNo, quint32 value);
  quint32 ethCudaGridSize(quint32 _deviceNo) const;

  // OCL device params
  quint32 clLocalWork(quint32 _deviceNo, HashFamily _family) const;
  quint32 clGlobalWork(quint32 _deviceNo, HashFamily _family) const;
  void setClLocalWork(quint32 _deviceNo, HashFamily _family, quint32 _value);
  void setClGlobalWork(quint32 _deviceNo, HashFamily _family, quint32 _value);

  QString dagDir() const;
  void setDagDir(const QString &_value);

  bool dagInMemory() const;
private:
  QScopedPointer<SettingsPrivate> d_ptr;

  Settings();
  ~Settings();
};

}
