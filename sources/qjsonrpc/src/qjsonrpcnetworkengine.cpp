
#include <QUrl>
#include <QTcpSocket>

#include "qjsonrpc/qjsonrpcnetworkengine.h"

namespace JsonRpc {

class JsonRpcNetworkEnginePrivate {
public:
  JsonRpcNetworkEnginePrivate(const QUrl &_url) : m_url(_url) {
  }

  QUrl m_url;
};

JsonRpcNetworkEngine::JsonRpcNetworkEngine(const QUrl &_url) : QObject(), d_ptr(new JsonRpcNetworkEnginePrivate(_url)) {
}

JsonRpcNetworkEngine::~JsonRpcNetworkEngine() {
}

QUrl JsonRpcNetworkEngine::url() const {
  return d_ptr->m_url;
}

void JsonRpcNetworkEngine::sendData(const QByteArray &_data) {
  sendDataImpl(_data);
}

class TcpNetworkEnginePrivate {
public:
  TcpNetworkEnginePrivate() : m_sock() {
  }

  QTcpSocket m_sock;
};

TcpNetworkEngine::TcpNetworkEngine(const QUrl &_url) : JsonRpcNetworkEngine(_url) {
}

TcpNetworkEngine::~TcpNetworkEngine() {
}

void TcpNetworkEngine::sendDataImpl(const QByteArray &_data) {
}

}
