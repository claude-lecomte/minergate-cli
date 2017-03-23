#include <QProcess>
#include <QCoreApplication>
#include <QVariant>
#include <QSettings>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QDataStream>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "utils/settings_p.h"
#include "utils/environment.h"
#include "utils/settings.h"
#include "utils/logger.h"
#include "utils/familyinfo.h"

namespace MinerGate {

const QString STARTING_BM("startingBM");
const QString TEST_DATA("testData");
const QString SUPPRESS_ACHIEVEMENT_NOTIFY("suppress_achievement_notify");
const QString GPU_INTENSITY("gpu_intencity");
const QString GPU_STATE("gpu_state");
const QString ACHIEVEMENTS_SUFFIX("achievements");
const QString ACHIEVEMENTS_5_07("achievements");
const QString ACHIEVEMENTS("achievements_data");
const QString SEED_HASH("seedHash");
const QString BLOCK_SIZE("blockSize");
const QString GRID_SIZE("gridSize");
const QString LOCAL_WORK("clLocalWork");
const QString GLOBAL_WORK("clGlobalWork");
const QString CUDA("CUDA-ETH");
const QString OCL("OCL");
const QString DAG_DIR("dagDir");
const QString DAG_IN_MEMORY("dagInMemory");

const QString DEFAULT_DAG_DIR(".ethash-minergate");

const quint32 CUDA_DEFAULT_GRIDSIZE = 32;

const quint32 OCL_DEFAULT_LOCAL_WORK_SIZE_CRYPTONOTE(16);
const quint32 OCL_DEFAULT_GLOBAL_WORK_SIZE_MULTIPLIER_CRYPTONOTE(16);

const quint32 OCL_DEFAULT_LOCAL_WORK_SIZE_ETH(64);
const quint32 OCL_DEFAULT_GLOBAL_WORK_SIZE_MULTIPLIER_ETH(4096);

const quint16 MAX_ACHIEVEMENTS_SAVE_COUNT(20);

class Group {
public:
  Group(const QString &_groupName) {
    Settings::instance().d_ptr->m_settingsImpl->beginGroup(_groupName);
  }
  ~Group() {
    Settings::instance().d_ptr->m_settingsImpl->endGroup();
  }
private:
};

Settings::Settings() : d_ptr(new SettingsPrivate) {
  Q_D(Settings);
  d->m_settingsImpl.reset(new MemorySettings);
}

Settings::~Settings() {
}

QVariantMap Settings::minerList() const {
  Q_D(const Settings);
  Group gMiners("miners");
  QStringList miners = d->m_settingsImpl->childGroups();
  QVariantMap res;
  Q_FOREACH (const auto &miner, miners) {
    QVariantMap miner_map;
    miner_map["engine"] = miner;
    Group gMiner(miner);
    {
      QVariantMap merged_pools;
      {
        Group gMergedPools("mergedPools");
        QStringList merged_coins = d->m_settingsImpl->childKeys();
        Q_FOREACH (const auto &coin, merged_coins) {
          merged_pools.insert(coin, d->m_settingsImpl->value(coin));
        }
      }

      QStringList keys = d->m_settingsImpl->childKeys();
      Q_FOREACH (const auto &key, keys)
        miner_map.insert(key, d->m_settingsImpl->value(key));

      miner_map.insert("mergedPools", merged_pools);
    }

    res.insert(miner, miner_map);
  }

  return res;
}

QVariantMap Settings::customMinerList() const {
  Q_D(const Settings);
  Group gMiners("customMiners");
  QStringList miners = d->m_settingsImpl->childGroups();
  QVariantMap res;
  Q_FOREACH (const auto &miner, miners) {
    QVariantMap miner_map;
    miner_map["engine"] = miner;
    Group gMiner(miner);
    {
      QVariantMap merged_pools;
      QStringList keys = d->m_settingsImpl->childKeys();
      Q_FOREACH (const auto &key, keys) {
        miner_map.insert(key, d->m_settingsImpl->value(key));
      }
    }

    res.insert(miner, miner_map);
  }

  return res;
}

bool Settings::startOnLoginEnabled() const {
  bool res = false;
#ifdef Q_OS_MAC
  QDir autorun_dir = QDir::home();
  if (!autorun_dir.cd("Library") || !autorun_dir.cd("LaunchAgents")) {
    return false;
  }

  QString autorun_file_path = autorun_dir.absoluteFilePath("minergate.plist");
  if (!QFile::exists(autorun_file_path)) {
    return false;
  }

  QSettings autorun_settings(autorun_file_path, QSettings::NativeFormat);
  res = autorun_settings.value("RunAtLoad", false).toBool();
#elif defined(Q_OS_LINUX)
  QString config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (config_path.isEmpty()) {
    return false;
  }

  QDir autorun_dir(config_path);
  if (!autorun_dir.cd("autostart")) {
    return false;
  }

  QString autorun_file_path = autorun_dir.absoluteFilePath("minergate.desktop");
  res = QFile::exists(autorun_file_path);

#elif defined(Q_OS_WIN)
  QSettings autorun_settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString value = autorun_settings.value("MinerGateGui").toString();
  if (value.endsWith("--auto"))
    value.chop(6);
  value = value.trimmed();
  res = autorun_settings.contains("MinerGateGui") &&
    !QDir::fromNativeSeparators(value).compare(QCoreApplication::applicationFilePath());
#endif
  return res;
}

void Settings::setStartOnLoginEnabled(bool _on) {
#ifdef Q_OS_MAC
  QDir autorunDir = QDir::home();
  if (!autorunDir.cd("Library"))
    return;
  if (!autorunDir.cd("LaunchAgents")) {
    autorunDir.mkdir("LaunchAgents");
    if (!autorunDir.cd("LaunchAgents"))
      return;
  }

  const QString &autorunFilePath = autorunDir.absoluteFilePath("minergate.plist");
  QSettings autorunSettings(autorunFilePath, QSettings::NativeFormat);
  autorunSettings.remove("Program");
  autorunSettings.setValue("Label", "com.minergate");
  const QString &appPath = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first() + "/MinerGate.app/Contents/MacOS/MinerGate";
  autorunSettings.setValue("ProgramArguments", QVariantList() << appPath << "--auto");
  autorunSettings.setValue("RunAtLoad", _on);
  autorunSettings.setValue("ProcessType", "InterActive");
#elif defined(Q_OS_LINUX)
  QString config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (config_path.isEmpty()) {
    return;
  }

  QDir autorun_dir(config_path);
  if(!autorun_dir.exists("autostart")) {
    autorun_dir.mkdir("autostart");
  }

  if (!autorun_dir.cd("autostart")) {
    return;
  }

  QString autorun_file_path = autorun_dir.absoluteFilePath("minergate.desktop");
  QFile autorun_file(autorun_file_path);
  if (!autorun_file.open(QFile::WriteOnly | QFile::Truncate)) {
    return;
  }

  if (_on) {
    autorun_file.write("[Desktop Entry]\n");
    autorun_file.write("Type=Application\n");
    autorun_file.write("Hidden=false\n");
    autorun_file.write(QString("Exec=%1 --auto\n").arg(QCoreApplication::applicationFilePath()).toLocal8Bit());
    autorun_file.write("Name=Minergate\n");
    autorun_file.write("Terminal=false\n");
    autorun_file.close();
  } else {
    QFile::remove(autorun_file_path);
  }
#elif defined(Q_OS_WIN)
  QSettings autorun_settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  if (_on) {
    const QString app(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    autorun_settings.setValue("MinerGateGui", app + " --auto");
  } else {
    autorun_settings.remove("MinerGateGui");
  }
#endif
}

bool Settings::addMiner(const QVariantMap &_params) {
  Q_D(Settings);
  QVariantMap params(_params);
  Group gMiner("miners");
  {
    Group gEngine(params.take("engine").toString());
    {
      for (QVariantMap::iterator it = params.begin(); it != params.end(); ++it) {
        if (it.value().isValid()) {
          d->m_settingsImpl->setValue(it.key(), it.value());
        } else {
          d->m_settingsImpl->remove(it.key());
        }
      }
    }
  }
  return true;
}

bool Settings::addCustomMiner(const QVariantMap &_params) {
  Q_D(Settings);
  QVariantMap params(_params);
  Group gMiners("customMiners");
  {
    Group gEngine(params.take("engine").toString());
    {
      for (QVariantMap::iterator it = params.begin(); it != params.end(); ++it) {
        d->m_settingsImpl->setValue(it.key(), it.value());
      }
    }
  }
  return true;
}

bool Settings::removeCustomMiner(const QString &_key) {
  Q_D(Settings);
  Group g("customMiners");
  d->m_settingsImpl->remove(_key);
  return true;
}

QString Settings::language() const {
  Q_D(const Settings);
  QString res = d->m_settingsImpl->value("language", QString()).value<QString>();
  return res;
}

void Settings::setLanguage(const QString &_lang) {
  Q_D(Settings);
  d->m_settingsImpl->setValue("language", _lang);
}

void Settings::writeParam(const QString &_name, const QVariant &_value) {
  Q_D(Settings);
  d->m_settingsImpl->setValue(_name, _value);
}

QVariant Settings::readParam(const QString &_name, const QVariant &_default) const {
  Q_D(const Settings);
  return d->m_settingsImpl->value(_name, _default);
}

void Settings::writeMinerParam(const QString &_coin_abbr, const QString &_name, bool _custom, const QVariant &_value) {
  Q_D(Settings);
  Group gMiners(_custom ? "customMiners" : "miners");
  {
    Group gCoin(_coin_abbr);
    d->m_settingsImpl->setValue(_name, _value);
  }
}

QVariant Settings::readMinerParam(const QString &_coin_abbr, const QString &_name, bool _custom, const QVariant &_default) const {
  Q_D(const Settings);
  QVariant res;
  Group gMiners(_custom ? "customMiners" : "miners");
  {
    Group gCoin(_coin_abbr);
    res = d->m_settingsImpl->value(_name, _default);
  }
  return res;
}

quint8 Settings::gpuIntensity(const QString &_coin_abbr, const QString _deviceKey) const {
  Q_D(const Settings);
  Group gIntensity(GPU_INTENSITY);
  Group gCoin(_coin_abbr);
  const quint8 DEFAULT_GPU_INTENSITY(2);
  quint8 value = d->m_settingsImpl->value(_deviceKey, DEFAULT_GPU_INTENSITY).value<quint8>();
  return value;
}

void Settings::setGpuIntensity(const QString &_coin_abbr, const QString _deviceKey, quint8 value) {
  Q_D(Settings);
  Q_ASSERT(!_coin_abbr.isEmpty());
  Q_ASSERT(!_deviceKey.isEmpty());
  Q_ASSERT(value >= 1 && value <= 4);
  Group gIntensity(GPU_INTENSITY);
  Group gCoin(_coin_abbr);
  d->m_settingsImpl->setValue(_deviceKey, value);
}

bool Settings::isGpuRunning(const QString &_coin_abbr, const QString _deviceKey) const {
  Q_D(const Settings);
  Group gState(GPU_STATE);
  Group gCoin(_coin_abbr);
  bool value = false;
  if (_deviceKey.isEmpty()) {
    // return true, if at least one device is running
    Q_FOREACH (const QString &key, d->m_settingsImpl->childKeys()) {
      if (d->m_settingsImpl->value(key, false).toBool()) {
        value = true;
        break;
      }
    }
  } else
    value = d->m_settingsImpl->value(_deviceKey, false).toBool();
  return value;
}

void Settings::setGpuRunning(const QString &_coin_abbr, const QString _deviceKey, bool _value) {
  Q_D(Settings);
  if (d->m_appExitInProcess)
    return;
  Q_ASSERT(!_coin_abbr.isEmpty());
  Q_ASSERT(!_deviceKey.isEmpty());
  Group gState(GPU_STATE);
  Group gCoin(_coin_abbr);
  if (d->m_settingsImpl->value(_deviceKey).value<quint8>() != _value)
    d->m_settingsImpl->setValue(_deviceKey, _value);
}

QStringList Settings::childKeys(const QString &_section) const {
  Q_D(const Settings);
  Group g(_section);
  QStringList res = d->m_settingsImpl->childKeys();
  return res;
}

bool Settings::wasStartingBenchmark() const {
  Q_D(const Settings);
  return d->m_settingsImpl->value(STARTING_BM, false).toBool();
}

void Settings::setStartingBenchmark(bool was) {
  Q_D(Settings);
  d->m_settingsImpl->setValue(STARTING_BM, was);
}

void Settings::saveTestData(const QByteArray &data) {
  Q_D(Settings);
  d->m_settingsImpl->setValue(TEST_DATA, data);
}

QByteArray Settings::testData() const {
  Q_D(const Settings);
  return d->m_settingsImpl->value(TEST_DATA).toByteArray();
}

void Settings::clearTestData() {
  Q_D(Settings);
  d->m_settingsImpl->remove(TEST_DATA);
}

void Settings::saveAchievements(const QByteArray &_data) {
  Q_D(Settings);
  const QString &login = d->m_settingsImpl->value("email").toString();
  const QString fileName(Env::dataDir().absoluteFilePath(QString("%1.%2").arg(login).arg(ACHIEVEMENTS_SUFFIX)));
  // check for rotate file
  static quint16 saveCount = 10;
  if (saveCount++ > MAX_ACHIEVEMENTS_SAVE_COUNT) {
    saveCount = 0;
    // rotate
    const QString &bakFileName = fileName + ".bak";
    if (QFile::exists(bakFileName))
      QFile::remove(bakFileName);
    QFile::rename(fileName, bakFileName);
  }

  QFile file(fileName);
  const QString saveError("Can't save achievements, sorry.");
  if (!file.open(QFile::WriteOnly)) {
    Logger::critical(QString::null, saveError);
    return;
  }
  QDataStream ds(&file);
  QString appVersion = Settings::instance().readParam("version").toString();
  appVersion.replace(".", "");
  quint16 uiVer = appVersion.toUInt();
  ds << uiVer << _data;
}

QByteArray Settings::achievements(quint32 &uiVer) const {
  Q_D(const Settings);
  const QString &login = d->m_settingsImpl->value("email").toString();
  QString appVersion = d->m_migratedFrom;
  if (appVersion.isEmpty())
    appVersion = d->m_settingsImpl->value("version").toString();
  appVersion.replace(".", "");
  uiVer = appVersion.toUInt();

  switch (uiVer) {
  case 507:
  case 508:
    Logger::info(QString::null, QString("Achievements data was converted from format %1 to newest").arg(uiVer));
    return d->m_settingsImpl->value(QString("%1-%2")
                                .arg(uiVer==507?ACHIEVEMENTS_5_07:ACHIEVEMENTS)
                                .arg(login)).toByteArray();
  default: {
    // for 5.09 and later
    const QString fileName("%1/%2.%3");
    QFile file(fileName.arg(Env::dataDir().absolutePath()).arg(login).arg(ACHIEVEMENTS_SUFFIX));
    const QString saveError("Can't load achievements");
    if (!file.open(QFile::ReadOnly | QFile::Unbuffered)) {
      Logger::critical(QString::null, saveError);
      return QByteArray();
    }
    quint16 uiVer;
    QByteArray ar;
    {
      QDataStream ds(&file);
      ds >> uiVer >> ar;
    }
    file.close();
    return ar;
  }
  }
}

void Settings::setSuppressAchievementNotify(bool suppress) {
  Q_D(Settings);
  d->m_settingsImpl->setValue(SUPPRESS_ACHIEVEMENT_NOTIFY, suppress);
}

bool Settings::suppressAchievementNotify() const {
  Q_D(const Settings);
  return d->m_settingsImpl->value(SUPPRESS_ACHIEVEMENT_NOTIFY).toBool();
}

void Settings::sync() {
  Q_D(const Settings);
  d->m_settingsImpl->sync();
}

void Settings::setAppQuitInProcess(bool isQuit) {
  Q_D(Settings);
  d->m_appExitInProcess = isQuit;
}

bool Settings::appQuitInProcess() const {
  Q_D(const Settings);
  return d->m_appExitInProcess;
}

QString Settings::formatDateTime(const QDateTime &dateTime, bool _withSeconds) {
  if (_withSeconds)
    return dateTime.toString("dd.MM.yyyy hh:mm:ss");
  else
    return dateTime.toString("dd.MM.yyyy hh:mm");
}

QString Settings::formatDate(const QDate &date) {
  return date.toString("dd.MM.yyyy");
}

QString Settings::formatTime(const QTime &time) {
  return time.toString("hh:mm");
}

void Settings::ethCudaSetGridSize(quint32 _deviceNo, quint32 value) {
  Q_D(Settings);
  Group g(CUDA);
  Group g2(QString::number(_deviceNo));
  d->m_settingsImpl->setValue(GRID_SIZE, value);
}

quint32 Settings::ethCudaGridSize(quint32 _deviceNo) const {
  Q_D(const Settings);
  quint32 value;
  {
    Group g(CUDA);
    Group g2(QString::number(_deviceNo));
    value = d->m_settingsImpl->value(GRID_SIZE).value<quint32>();
  }
  if (!value) {
    Settings::instance().ethCudaSetGridSize(_deviceNo, CUDA_DEFAULT_GRIDSIZE);
    value = CUDA_DEFAULT_GRIDSIZE;
  }
  return value;
}

quint32 Settings::clLocalWork(quint32 _deviceNo, HashFamily _family) const {
  Q_D(const Settings);
  quint32 value;
  {
    Group g(OCL);
    Group g2(QString::number(_deviceNo));
    Group g3(QString::number((int)_family));
    value = d->m_settingsImpl->value(LOCAL_WORK).value<quint32>();
  }
  if (!value) {
    value = _family==HashFamily::Ethereum?OCL_DEFAULT_LOCAL_WORK_SIZE_ETH:OCL_DEFAULT_LOCAL_WORK_SIZE_CRYPTONOTE;
    Settings::instance().setClLocalWork(_deviceNo, _family, value);
  }
  return value;
}

quint32 Settings::clGlobalWork(quint32 _deviceNo, HashFamily _family) const {
  Q_D(const Settings);
  quint32 value;
  {
    Group g(OCL);
    Group g2(QString::number(_deviceNo));
    Group g3(QString::number((int)_family));
    value = d->m_settingsImpl->value(GLOBAL_WORK).value<quint32>();
  }
  if (!value) {
    value = _family==HashFamily::Ethereum?OCL_DEFAULT_GLOBAL_WORK_SIZE_MULTIPLIER_ETH:OCL_DEFAULT_GLOBAL_WORK_SIZE_MULTIPLIER_CRYPTONOTE;
    Settings::instance().setClGlobalWork(_deviceNo, _family, value);
  }
  return value;
}

void Settings::setClLocalWork(quint32 _deviceNo, HashFamily _family, quint32 _value) {
  Q_D(Settings);
  Group g(OCL);
  Group g2(QString::number(_deviceNo));
  Group g3(QString::number((int)_family));
  d->m_settingsImpl->setValue(LOCAL_WORK, _value);
}

void Settings::setClGlobalWork(quint32 _deviceNo, HashFamily _family, quint32 _value) {
  Q_D(Settings);
  Group g(OCL);
  Group g2(QString::number(_deviceNo));
  Group g3(QString::number((int)_family));
  d->m_settingsImpl->setValue(GLOBAL_WORK, _value);
}

QString Settings::dagDir() const {
  Q_D(const Settings);
  QString value = d->m_settingsImpl->value(DAG_DIR).toString();
  if (value.isEmpty()) {
#ifdef Q_OS_WIN
    value = Env::dataDir().absoluteFilePath(DEFAULT_DAG_DIR);
#else
    value = Env::homeDir().absoluteFilePath(DEFAULT_DAG_DIR);
#endif
    Settings::instance().setDagDir(value);
  }
  return value;
}

void Settings::setDagDir(const QString &_value) {
  Q_D(Settings);
  d->m_settingsImpl->setValue(DAG_DIR, _value);
}

bool Settings::dagInMemory() const {
  Q_D(const Settings);
  if (d->m_settingsImpl->childKeys().contains(DAG_IN_MEMORY))
    return d->m_settingsImpl->value(DAG_IN_MEMORY).toBool();
  d->m_settingsImpl->setValue(DAG_IN_MEMORY, false);
  return true;
}

void Settings::removeAchievementParam() {
  Q_D(Settings);
  const QString &login = d->m_settingsImpl->value("email").toString();
  d->m_settingsImpl->remove(QString("%1-%2").arg(ACHIEVEMENTS_5_07).arg(login));
  d->m_settingsImpl->remove(QString("%1-%2").arg(ACHIEVEMENTS).arg(login));
}

bool Settings::tryToRestoreAchievements() {
  Q_D(Settings);
  const QString &login = d->m_settingsImpl->value("email").toString();
  const QString fileName(Env::dataDir().absoluteFilePath(QString("%1.%2").arg(login).arg(ACHIEVEMENTS_SUFFIX)));
  const QString bakFileName(fileName + ".bak");
  if (!QFile::exists(bakFileName)) {
    Logger::debug(QString::null, "bak file is not exists");
    return false;
  }
  if (!QFile::remove(fileName)) {
    Logger::critical(QString::null, QString("can't remove achievements file %1").arg(fileName));
    return false;
  }
  if (!QFile::copy(bakFileName, fileName)) {
    Logger::critical(QString::null, QString("can't restore achievement from %1 to %2")
                     .arg(bakFileName).arg(fileName));
    return false;
  } else
    return true;
}

}
