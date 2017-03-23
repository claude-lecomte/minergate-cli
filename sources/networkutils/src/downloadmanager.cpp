
#include <QTimer>

#include "networkutils/downloadmanager.h"
#include "networkutils/networkenvironment.h"

#include "utils/environment.h"

#define H_CLIENTNAME "client-name"
#define H_OSFAMILY "client-os-family"
#define H_OSVERSION "client-os-version"
#define H_CLIENTVERSION "client-version"
#define H_CPUID "x-cpu-id"

namespace MinerGate {

class DownloadManagerPrivate {
public:
  QVariant m_user_data;
  QByteArray m_post_data;
  QByteArray m_buffer;
  QNetworkRequest m_request;


  QNetworkReply *m_reply;
  QTimer m_timer;
  QTimer m_request_timer;
};

DownloadManager::DownloadManager(const QUrl &_url, const QVariant &_user_data, quint32 _interval,
  const QByteArray &_post_data) : QObject(), d_ptr(new DownloadManagerPrivate) {
  d_ptr->m_request = QNetworkRequest(_url);
  d_ptr->m_user_data = _user_data;
  d_ptr->m_post_data = _post_data;
  d_ptr->m_reply = NULL;

  QStringList cpu_id_str_list;
  QList<quint32> cpu_id_list = Env::cpuId();
  Q_FOREACH (quint32 cpu_id, cpu_id_list) {
    cpu_id_str_list << QString::number(cpu_id, 16);
  }

  d_ptr->m_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  d_ptr->m_request.setRawHeader(H_CLIENTNAME, CLIENT_NAME);
  d_ptr->m_request.setRawHeader(H_OSFAMILY, Env::os().first.toUtf8());
  d_ptr->m_request.setRawHeader(H_OSVERSION, Env::os().second.toUtf8());
  d_ptr->m_request.setRawHeader(H_CLIENTVERSION, Env::version().toUtf8());
  d_ptr->m_request.setRawHeader(H_CPUID, cpu_id_str_list.join(',').toUtf8());

  if (_interval > 0) {
    d_ptr->m_timer.setInterval(_interval);
  } else {
    d_ptr->m_request_timer.setInterval(10000);
  }
}

DownloadManager::DownloadManager(const QNetworkRequest &_request, const QVariant &_user_data, quint32 _interval,
  const QByteArray &_post_data) : QObject(), d_ptr(new DownloadManagerPrivate) {
  d_ptr->m_request = _request;
  d_ptr->m_user_data = _user_data;
  d_ptr->m_post_data = _post_data;
  d_ptr->m_reply = NULL;

  QStringList cpu_id_str_list;
  QList<quint32> cpu_id_list = Env::cpuId();
  Q_FOREACH (quint32 cpu_id, cpu_id_list) {
    cpu_id_str_list << QString::number(cpu_id, 16);
  }

  d_ptr->m_request.setRawHeader(H_CLIENTNAME, CLIENT_NAME);
  d_ptr->m_request.setRawHeader(H_OSFAMILY, Env::os().first.toUtf8());
  d_ptr->m_request.setRawHeader(H_OSVERSION, Env::os().second.toUtf8());
  d_ptr->m_request.setRawHeader(H_CLIENTVERSION, Env::version().toUtf8());
  d_ptr->m_request.setRawHeader(H_CPUID, cpu_id_str_list.join(',').toUtf8());

  if (_interval > 0) {
    d_ptr->m_timer.setInterval(_interval);
  } else {
    d_ptr->m_request_timer.setInterval(10000);
  }
}

DownloadManager::~DownloadManager() {
  if (d_ptr->m_reply) {
    d_ptr->m_reply->deleteLater();
  }
}

void DownloadManager::setInterval(quint32 _interval) {
  d_ptr->m_timer.stop();
  d_ptr->m_timer.start(_interval);
}

quint32 DownloadManager::interval() const {
  return d_ptr->m_timer.interval();
}

bool DownloadManager::active() const {
  return d_ptr->m_timer.isActive();
}

void DownloadManager::get() {
  if (d_ptr->m_reply) {
    d_ptr->m_reply->abort();
    d_ptr->m_reply->deleteLater();
  }

  d_ptr->m_buffer.clear();
  d_ptr->m_reply = NetworkEnvironment::instance().get(d_ptr->m_request);

  connect(d_ptr->m_reply, &QIODevice::readyRead, this, &DownloadManager::readyRead);
  connect(d_ptr->m_reply, &QNetworkReply::finished, this, &DownloadManager::finished);
  connect(d_ptr->m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
    SLOT(error(QNetworkReply::NetworkError)), Qt::UniqueConnection);
  if (d_ptr->m_timer.interval() > 0 && !d_ptr->m_timer.isActive()) {
    connect(&d_ptr->m_timer, &QTimer::timeout, this, &DownloadManager::get, Qt::UniqueConnection);
    d_ptr->m_timer.start();
  }

  if (d_ptr->m_request_timer.interval() > 0) {
    connect(&d_ptr->m_request_timer, &QTimer::timeout, d_ptr->m_reply, &QNetworkReply::abort, Qt::UniqueConnection);
    connect(&d_ptr->m_request_timer, &QTimer::timeout, &d_ptr->m_request_timer, &QTimer::stop, Qt::UniqueConnection);
    d_ptr->m_request_timer.start();
  }
}

void DownloadManager::post() {
  if (d_ptr->m_reply) {
    d_ptr->m_reply->abort();
    d_ptr->m_reply->deleteLater();
  }

  d_ptr->m_buffer.clear();
  d_ptr->m_reply = NetworkEnvironment::instance().post(d_ptr->m_request, d_ptr->m_post_data);

  connect(d_ptr->m_reply, &QIODevice::readyRead, this, &DownloadManager::readyRead);
  connect(d_ptr->m_reply, &QNetworkReply::finished, this, &DownloadManager::finished);
  connect(d_ptr->m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
    SLOT(error(QNetworkReply::NetworkError)), Qt::UniqueConnection);
  if (d_ptr->m_timer.interval() > 0 && !d_ptr->m_timer.isActive()) {
    connect(&d_ptr->m_timer, &QTimer::timeout, this, &DownloadManager::post, Qt::UniqueConnection);
    d_ptr->m_timer.start();
  }

  if (d_ptr->m_request_timer.interval() > 0) {
    connect(&d_ptr->m_request_timer, &QTimer::timeout, d_ptr->m_reply, &QNetworkReply::abort, Qt::UniqueConnection);
    connect(&d_ptr->m_request_timer, &QTimer::timeout, &d_ptr->m_request_timer, &QTimer::stop, Qt::UniqueConnection);
    d_ptr->m_request_timer.start();
  }
}

void DownloadManager::pause() {
  d_ptr->m_timer.stop();
}

void DownloadManager::resume() {
  get();
}

void DownloadManager::abort() {
  if (d_ptr->m_reply) {
    d_ptr->m_timer.stop();
    d_ptr->m_request_timer.stop();
    d_ptr->m_reply->abort();
    d_ptr->m_reply->deleteLater();
    d_ptr->m_reply = nullptr;
    Q_EMIT abortSignal();
  }
}

void DownloadManager::readyRead() {
  QNetworkReply *reply = (QNetworkReply*)sender();
  QByteArray tmp(reply->bytesAvailable(), 0);
  reply->read(tmp.data(), tmp.size());
  d_ptr->m_buffer.append(tmp);
}

void DownloadManager::finished() {
  d_ptr->m_request_timer.stop();
  QNetworkReply *reply = (QNetworkReply*)sender();
  quint32 code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).value<quint32>();
  Q_EMIT finishedSignal(code, d_ptr->m_buffer, d_ptr->m_user_data);
}

void DownloadManager::error(QNetworkReply::NetworkError _error) {
  Q_EMIT errorSignal(_error, d_ptr->m_user_data);
}

}
