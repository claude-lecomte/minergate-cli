
#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "networkutilscommonprerequisites.h"

namespace MinerGate {

class DownloadManagerPrivate;

class DownloadManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(DownloadManager)
  Q_DECLARE_PRIVATE(DownloadManager)

public:
  DownloadManager(const QUrl &_url, const QVariant &_user_data, quint32 _interval = 0,
    const QByteArray &_post_data = QByteArray());
  DownloadManager(const QNetworkRequest &_request, const QVariant &_user_data, quint32 _interval = 0,
    const QByteArray &_post_data = QByteArray());
  ~DownloadManager();
  void setInterval(quint32 _interval);
  quint32 interval() const;
  bool active() const;
private:
  QScopedPointer<DownloadManagerPrivate> d_ptr;

public Q_SLOTS:
  void get();
  void post();
  void pause();
  void resume();
  void abort();

private Q_SLOTS:
  void readyRead();
  void finished();
  void error(QNetworkReply::NetworkError code);

Q_SIGNALS:
  void finishedSignal(quint32 _code, const QByteArray &_data, const QVariant &_user_data);
  void errorSignal(QNetworkReply::NetworkError _error, const QVariant &_user_data);
  void abortSignal();
};

}
