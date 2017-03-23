#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>

#include "networkutils/networkenvironment.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/environment.h"

namespace MinerGate {

class NetworkEnvironmentPrivate {
public:
  NetworkEnvironmentPrivate() : m_net_man(new QNetworkAccessManager) {
  }

  QNetworkAccessManager *m_net_man;
};

NetworkEnvironment::NetworkEnvironment() : d_ptr(new NetworkEnvironmentPrivate) {
  updateProxySettings();
}

NetworkEnvironment::~NetworkEnvironment() {
}

void NetworkEnvironment::connectToHost(const QString &_host, quint16 _port) {
  d_ptr->m_net_man->connectToHost(_host, _port);
}

QNetworkReply *NetworkEnvironment::get(const QNetworkRequest &_request) {
  QNetworkRequest req(_request);
  req.setHeader(QNetworkRequest::UserAgentHeader, Env::userAgent());
  return d_ptr->m_net_man ->get(req);
}

QNetworkReply *NetworkEnvironment::post(const QNetworkRequest &_request, const QByteArray &_data) {
  QNetworkRequest req(_request);
  req.setHeader(QNetworkRequest::UserAgentHeader, Env::userAgent());
  return d_ptr->m_net_man->post(req, _data);
}

void NetworkEnvironment::updateProxySettings() {
  QString set_type = Settings::instance().readParam("proxy", "no").toString();
  QNetworkProxy proxy;
  if (!set_type.compare("no")) {
    proxy.setType(QNetworkProxy::NoProxy);
  } else if (!set_type.compare("system")) {
    proxy = getSystemProxy();
  } else {
    QUrl proxy_url = Settings::instance().readParam("proxy-url").toUrl();

    if (proxy_url.isEmpty()) {
      proxy.setType(QNetworkProxy::NoProxy);
    } else {
      if (!proxy_url.scheme().compare("socks")) {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setCapabilities(proxy.capabilities() & ~QNetworkProxy::HostNameLookupCapability);
      } else
        proxy.setType(QNetworkProxy::NoProxy);

      if (proxy_url.host().isEmpty()) {
        proxy.setType(QNetworkProxy::NoProxy);
      } else {
        proxy.setHostName(proxy_url.host());
      }

      if (proxy_url.port() == 0) {
        proxy.setType(QNetworkProxy::NoProxy);
      } else {
        proxy.setPort(proxy_url.port());
      }
    }
  }

  QNetworkProxy::setApplicationProxy(proxy);
  Q_EMIT updateProxySignal();
}

QNetworkProxy NetworkEnvironment::getSystemProxy() {
  QList<QNetworkProxy> proxy_list = QNetworkProxyFactory::systemProxyForQuery();
  QNetworkProxy res(QNetworkProxy::NoProxy);
  Q_FOREACH (const auto &proxy, proxy_list) {
    switch (proxy.type()) {
    case QNetworkProxy::Socks5Proxy:
      return proxy;
    default:
      break;
    }
  }

  return res;
}

}
