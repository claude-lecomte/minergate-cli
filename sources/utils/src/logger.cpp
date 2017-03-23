
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QTimerEvent>

#include "utils/logger.h"
#include "utils/environment.h"
#include "utils/settings.h"

namespace MinerGate
{

const quint64 LOG_ROTATE_INTERVAL(1000 * 60 * 60); // hour
const quint64 MAX_LOG_SIZE(50 * 1024 * 1024); // 50 Mb max

class LoggerPrivate {
public:
  QFile m_log_file;
  TextStreamPtr m_log_stream;
  QMap<QString, QPair<FilePtr, TextStreamPtr> > m_log_channels;
  bool m_disable_log_files;
  int m_timerId = 0;

  QFile *fileForChannel(const QString &_channel) {
    if (_channel.isEmpty())
      return &m_log_file;
    else {
      QString channel = _channel.toLower();
      if (channel == "mro")
        channel = "xmr";
      else if (channel == "duck")
        channel = "xdn";

      if (m_log_channels.contains(channel))
        return m_log_channels.value(channel).first.data();
    }
    return nullptr;
  }

  static bool resetLogFile(QFile *_file, TextStreamPtr &_stream, const QString &_channelName) {
    Q_ASSERT(_file);
    _file->setFileName(Env::logDir().absoluteFilePath(QString("%1.log").arg(_channelName)));
    if (_file->open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
      _stream.reset(new QTextStream(_file));
      return true;
    }
    return false;
  }
};

void Logger::debug(const QString &_channel, const QString &_msg) {
  Logger::instance().debugMsg(_channel, _msg);
}

void Logger::info(const QString &_channel, const QString &_msg) {
  Logger::instance().infoMsg(_channel, _msg);
}

void Logger::warning(const QString &_channel, const QString &_msg) {
  Logger::instance().warningMsg(_channel, _msg);
}

void Logger::critical(const QString &_channel, const QString &_msg) {
  Logger::instance().criticalMsg(_channel, _msg);
}

void Logger::fatal(const QString &_channel, const QString &_msg) {
  Logger::instance().fatalMsg(_channel, _msg);
}

void Logger::disableLogFiles(bool disable) {
  Logger::instance().d_ptr->m_disable_log_files = disable;
}

Logger::Logger() :
  QObject(),
  d_ptr(new LoggerPrivate) {
  Env::logDir().mkpath(".");
  d_ptr->resetLogFile(&d_ptr->m_log_file, d_ptr->m_log_stream, "minergate");
  d_ptr->m_disable_log_files = false;
  d_ptr->m_timerId = startTimer(LOG_ROTATE_INTERVAL, Qt::VeryCoarseTimer);
}

Logger::~Logger() {
  killTimer(d_ptr->m_timerId);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
#ifdef QT_DEBUG
  QString formatted = QString("[%1] %2").arg((quint64)(QThread::currentThread())).arg(msg);
  QByteArray localMsg = formatted.toLocal8Bit();
#else
  QByteArray localMsg = msg.toLocal8Bit();
#endif
  switch (type) {
  case QtDebugMsg:
    fprintf(stderr, "%s\n", localMsg.constData());
    break;
  case QtWarningMsg:
#ifdef QT_DEBUG
    fprintf(stderr, "%s\n", localMsg.constData());
#endif
    break;
  case QtCriticalMsg:
#ifdef QT_DEBUG
    fprintf(stderr, "%s\n", localMsg.constData());
#endif
    break;
  case QtFatalMsg:
    fprintf(stderr, "%s\n", localMsg.constData());
    abort();
    break;
  default: break;
  }
}

void Logger::debugMsg(const QString &_channel, const QString &_msg) {
#ifdef QT_DEBUG
  QString msg = createLogString(QDateTime::currentDateTime(), "debug", _msg);
  qDebug("%s", msg.toLocal8Bit().data());
  QMutexLocker lock(&m_mutex);
  if (!d_ptr->m_disable_log_files) {
    *getStream(_channel.toLower()) << msg << endl;
  }
#endif
}

void Logger::infoMsg(const QString &_channel, const QString &_msg) {
  QString msg = createLogString(QDateTime::currentDateTime(), "info", _msg);
#if defined(Q_OS_WIN)
  qDebug("%s", msg.toLocal8Bit().data());
#else
  qDebug("\033[036m%s\033[0m", msg.toLocal8Bit().data());
#endif
  QMutexLocker lock(&m_mutex);
  if (!d_ptr->m_disable_log_files) {
    *getStream(_channel.toLower()) << msg << endl;
  }
}

void Logger::warningMsg(const QString &_channel, const QString &_msg) {
  QString msg = createLogString(QDateTime::currentDateTime(), "warn", _msg);
#if defined(Q_OS_WIN)
  qDebug("%s", msg.toLocal8Bit().data());
#else
  qDebug("\033[1;33m%s\033[0m", msg.toLocal8Bit().data());
#endif
  QMutexLocker lock(&m_mutex);
  if (!d_ptr->m_disable_log_files) {
    *getStream(_channel.toLower()) << msg << endl;
  }
}

void Logger::criticalMsg(const QString &_channel, const QString &_msg) {
  QString msg = createLogString(QDateTime::currentDateTime(), "error", _msg);
#if defined(Q_OS_WIN)
  qDebug("%s", msg.toLocal8Bit().data());
#else
  qDebug("\033[31m%s\033[0m", msg.toLocal8Bit().data());
#endif
  QMutexLocker lock(&m_mutex);
  if (!d_ptr->m_disable_log_files) {
    *getStream(_channel.toLower()) << msg << endl;
    QFile *logFile = d_ptr->fileForChannel(_channel);
    if (logFile)
      logFile->flush();
  }
}

void Logger::fatalMsg(const QString &_channel, const QString &_msg) {
  QString msg = createLogString(QDateTime::currentDateTime(), "fatal", _msg);
  QMutexLocker lock(&m_mutex);
  if (!d_ptr->m_disable_log_files) {
    *getStream(_channel.toLower()) << msg << endl;
    QFile *logFile = d_ptr->fileForChannel(_channel);
    if (logFile)
      logFile->flush();
  }
#if defined(Q_OS_WIN)
  qFatal("%s", msg.toLocal8Bit().data());
#else
  qFatal("\033[31m%s\033[0m", msg.toLocal8Bit().data());
#endif
}

QString Logger::createLogString(const QDateTime &_date_time, const QString &_log_cat, const QString &_msg) {
  return QString("[%1] [%2]  %3")
      .arg(Settings::formatDateTime(_date_time, true))
      .arg(_log_cat, 5)
      .arg(_msg);
}

TextStreamPtr Logger::getStream(const QString &_channel) const {
  if (_channel.isEmpty()) {
    return d_ptr->m_log_stream;
  }

  QString channel = _channel.toLower();
  if (channel == "mro")
    channel = "xmr";
  else if (channel == "duck")
    channel = "xdn";

  if (!d_ptr->m_log_channels.contains(channel)) {
    FilePtr file(new QFile(Env::logDir().absoluteFilePath(QString("%1.log").arg(channel))));
    TextStreamPtr str;
    if (d_ptr->resetLogFile(file.data(), str, channel))
      d_ptr->m_log_channels.insert(channel, qMakePair(file, str));
    else
      return d_ptr->m_log_stream;
  }

  return d_ptr->m_log_channels.value(channel).second;
}

void Logger::timerEvent(QTimerEvent *event) {
  QMutexLocker l(&m_mutex);

  if (d_ptr->m_disable_log_files)
    return;

  auto checkRotate = [this] (QFile &_file, TextStreamPtr &_stream, const QString &_channel) -> bool {
    // check for rotation of logs
    if (_file.size() < MAX_LOG_SIZE)
      return false;
    // required change
    _file.close();
    _stream.clear();
    // remove old file
    const QString &secondFile = _file.fileName() + ".old";
    if (QFile::exists(secondFile))
      if (!QFile::remove(secondFile))
        return false;
    _file.rename(secondFile);
    d_ptr->resetLogFile(&_file, _stream, _channel);
    return true;
  };

  checkRotate(d_ptr->m_log_file, d_ptr->m_log_stream, "minergate");
  for (auto it = d_ptr->m_log_channels.constBegin(); it != d_ptr->m_log_channels.constEnd(); ++it) {
    const QString &channel = it.key();
    const QPair<FilePtr, TextStreamPtr> &pair = it.value();
    TextStreamPtr newStream;
    if (checkRotate(*pair.first.data(), newStream, channel))
      d_ptr->m_log_channels.insert(channel, qMakePair(pair.first, newStream));
  }
}

}
