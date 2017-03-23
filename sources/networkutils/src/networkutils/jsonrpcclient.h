
#pragma once

#include <QAbstractSocket>

#include "networkutilscommonprerequisites.h"

namespace MinerGate {

class JsonRpcClientPrivate;

class JsonRpcClient : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(JsonRpcClient)
  Q_DECLARE_PRIVATE(JsonRpcClient)

public:
  JsonRpcClient(const QString &_host, quint16 _port);
  ~JsonRpcClient();

  void send(const QJsonObject &_obj);
  void reset();

private:
  QScopedPointer<JsonRpcClientPrivate> d_ptr;

  void connectToHost(const QString &_host, quint16 _port);

private Q_SLOTS:
  void connectedToHost();
  void disconnectedFromHost();
  void readyRead();
  void error(QAbstractSocket::SocketError _error);

Q_SIGNALS:
  void connectedToHostSignal();
  void disconnectedFromHostSignal();
  void dataSignal(const QJsonObject &_obj);
  void errorSignal(QAbstractSocket::SocketError _error);
};

}
