#pragma once

#include <QObject>
#include <QNetworkProxy>

#include "networkutilscommonprerequisites.h"

class QBuffer;

namespace MinerGate {

class NetworkEnvironmentPrivate;

class NetworkEnvironment : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(NetworkEnvironment)
  Q_DECLARE_PRIVATE(NetworkEnvironment)

public:
  static NetworkEnvironment &instance() {
    static NetworkEnvironment inst;
    return inst;
  }

  ~NetworkEnvironment();

  void connectToHost(const QString &_host, quint16 _port);

  QNetworkReply *get(const QNetworkRequest &_request);
  QNetworkReply *post(const QNetworkRequest &_request, const QByteArray &_data);
  QList<QNetworkProxy::ProxyType> proxies() const;
  void updateProxySettings();

private:
  QScopedPointer<NetworkEnvironmentPrivate> d_ptr;
  NetworkEnvironment();

  QNetworkProxy getSystemProxy();

Q_SIGNALS:
  void updateProxySignal();
};

}
