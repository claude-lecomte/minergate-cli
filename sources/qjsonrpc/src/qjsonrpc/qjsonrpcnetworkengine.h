
#pragma once

#include <QObject>

#include "qjsonrpccommonprerequisites.h"

namespace JsonRpc {

class JsonRpcNetworkEnginePrivate;

class JsonRpcNetworkEngine : public QObject {
  Q_OBJECT

public:
  JsonRpcNetworkEngine(const QUrl &_url);
  ~JsonRpcNetworkEngine();

public Q_SLOTS:
  void sendData(const QByteArray &_data);

protected:
  QUrl url() const;

  virtual void sendDataImpl(const QByteArray &_data) = 0;

private:
  QScopedPointer<JsonRpcNetworkEnginePrivate> d_ptr;

Q_SIGNALS:
  void dataSignal(const QByteArray &_data);
  void errorSignal(const QString &_error);
};


class TcpNetworkEnginePrivate;

class TcpNetworkEngine : public JsonRpcNetworkEngine {
  Q_OBJECT

public:
  TcpNetworkEngine(const QUrl &_url);
  ~TcpNetworkEngine();

protected:
  void sendDataImpl(const QByteArray &_data);

private:
  QScopedPointer<TcpNetworkEnginePrivate> d_ptr;
};

}
