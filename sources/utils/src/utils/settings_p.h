#pragma once

#include <QScopedPointer>
#include <QSettings>
#include <QStack>

#include "environment.h"

namespace MinerGate {

class AbstractSettings {
public:
  virtual ~AbstractSettings() {
  }

  virtual void beginGroup(const QString &prefix) = 0;
  virtual void endGroup() = 0;
  virtual QStringList childGroups() const = 0;
  virtual void setValue(const QString &key, const QVariant &value) = 0;
  virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const = 0;
  virtual QStringList childKeys() const = 0;
  virtual QStringList allKeys() const = 0;
  virtual void remove(const QString &key) = 0;
  virtual void sync() {}
};

class FileSettings: public AbstractSettings {
public:
  FileSettings(const QString &_fileName, QSettings::Format _format) {
    m_settings.reset(new QSettings(_fileName, _format));
  }

  ~FileSettings() {
  }

  void beginGroup(const QString &prefix) Q_DECL_OVERRIDE {
    m_settings->beginGroup(prefix);
  }

  void endGroup() Q_DECL_OVERRIDE {
    m_settings->endGroup();
  }

  QStringList childGroups() const Q_DECL_OVERRIDE {
    return m_settings->childGroups();
  }

  void setValue(const QString &key, const QVariant &value) Q_DECL_OVERRIDE {
    m_settings->setValue(key, value);
  }

  QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const Q_DECL_OVERRIDE {
    return m_settings->value(key, defaultValue);
  }

  QStringList childKeys() const Q_DECL_OVERRIDE {
    return m_settings->childKeys();
  }

  void remove(const QString &key) Q_DECL_OVERRIDE {
    m_settings->remove(key);
  }

  void sync() Q_DECL_OVERRIDE {
    m_settings->sync();
  }

  QStringList allKeys() const Q_DECL_OVERRIDE {
    return m_settings->allKeys();
  }

private:
  QScopedPointer<QSettings> m_settings;
};

class MemorySettings: public AbstractSettings {
public:
  MemorySettings() {
  }

  ~MemorySettings() {
  }

  void beginGroup(const QString &prefix) Q_DECL_OVERRIDE {
    m_groups.push(prefix);
  }

  void endGroup() Q_DECL_OVERRIDE {
    m_groups.pop();
  }

  QStringList childGroups() const Q_DECL_OVERRIDE {
    QStringList groups;
    const QString &curGroup = buildPrefix() + '\\';
    Q_FOREACH (QString realKey, m_settings.keys()) {
      if (!realKey.startsWith(curGroup))
        continue;
      realKey.remove(curGroup);
      const QStringList &path = realKey.split('\\');
      if (path.count() > 1)
        groups << path.first();
    }
    return groups;
  }

  void setValue(const QString &key, const QVariant &value) Q_DECL_OVERRIDE {
    const QString &realKey = buildPrefix(key);
    m_settings.insert(realKey, value);
  }

  QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const Q_DECL_OVERRIDE {
    const QString &realKey = buildPrefix(key);
    return m_settings.value(realKey, defaultValue);
  }

  QStringList childKeys() const Q_DECL_OVERRIDE {
    QStringList keys;
    const QString &curGroup = buildPrefix() + '\\';
    Q_FOREACH (QString realKey, m_settings.keys()) {
      if (!realKey.startsWith(curGroup))
        continue;
      realKey.remove(curGroup);
      const QStringList &path = realKey.split('\\');
      if (path.count() == 1)
        keys << realKey;
    }
    return keys;
  }

  QStringList allKeys() const Q_DECL_OVERRIDE {
    Q_UNIMPLEMENTED();
    return QStringList();
  }

  void remove(const QString &key) Q_DECL_OVERRIDE {
    const QString &realKey = buildPrefix(key);
    m_settings.remove(realKey);
  }
private:
  QHash<QString, QVariant> m_settings;
  QStack<QString> m_groups;

  QString buildPrefix(const QString &key = QString()) const {
    QStringList result;
    Q_FOREACH (const QString &prefix, m_groups)
      result << prefix;
    const QString &prefix = result.join('\\');
    return prefix.isEmpty()?key:QString("%1\\%2").arg(prefix).arg(key);
  }
};

class SettingsPrivate {
public:
  QScopedPointer<AbstractSettings> m_settingsImpl;
  bool m_appExitInProcess = false;
  QString m_migratedFrom;
  QString m_defaultPoolConfig;
};

}
