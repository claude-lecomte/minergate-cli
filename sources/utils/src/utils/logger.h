
#pragma once

#include <QObject>
#include <QMutex>

#include "utilscommonprerequisites.h"

namespace MinerGate {

class LoggerPrivate;

class Logger : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Logger)
  Q_DECLARE_PRIVATE(Logger)

public:
  static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

  static void debug(const QString &_channel, const QString &_msg);
  static void info(const QString &_channel, const QString &_msg);
  static void warning(const QString &_channel, const QString &_msg);
  static void critical(const QString &_channel, const QString &_msg);
  static void fatal(const QString &_channel, const QString &_msg);
  static void disableLogFiles(bool disable = true);

protected:
  void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
  static inline Logger &instance() {
    static Logger inst;
    return inst;
  }

  Logger();
  ~Logger();
  void resetLogFile();

  void debugMsg(const QString &_channel, const QString &_msg);
  void infoMsg(const QString &_channel, const QString &_msg);
  void warningMsg(const QString &_channel, const QString &_msg);
  void criticalMsg(const QString &_channel, const QString &_msg);
  void fatalMsg(const QString &_channel, const QString &_msg);
  static QString createLogString(const QDateTime &_date_time, const QString &_log_cat, const QString &_msg);
  TextStreamPtr getStream(const QString &_channel) const;

  QScopedPointer<LoggerPrivate> d_ptr;
  QMutex m_mutex;
};

}
