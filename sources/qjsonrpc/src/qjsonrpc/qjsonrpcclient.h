
#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QAbstractSocket>
#include <QNetworkProxy>

#include "qjsonrpccommonprerequisites.h"

namespace JsonRpc {

class JsonRpcMethodProxy : public QObject {
  Q_OBJECT

public:
  JsonRpcMethodProxy(QObject *_adapter, const QMetaMethod &_method);
  ~JsonRpcMethodProxy();

private:
  const QMetaMethod m_method;
  QString m_rpc_method;

private Q_SLOTS:
  void method();
  void method(const QVariantMap &_args);
  void method(const QVariantList &_args);

Q_SIGNALS:
  void methodSignal(const QString &_method);
  void methodSignal(const QString &_method, const QVariantMap &_args);
  void methodSignal(const QString &_method, const QVariantList &_args);
};

class JsonRpcQueueHandlerPrivate;

class JsonRpcQueueHandler : public QObject {
  Q_OBJECT

public:
  JsonRpcQueueHandler();
  ~JsonRpcQueueHandler();

  void addRequest(const QJsonObject &_obj);
  QJsonObject getRequestForProcess();
  QJsonObject removeRequest(quint64 _id);
  void clear(bool _full = true);

private:
  QScopedPointer<JsonRpcQueueHandlerPrivate> d_ptr;
};

class JsonRpcClientPrivate;

class JsonRpcClient : public QObject {
  Q_OBJECT

public:
  JsonRpcClient(const QUrl &_url, Version _version = Version::JsonRpc20);
  ~JsonRpcClient();

  QUrl url() const;

  void setAdapter(JsonRpcAdapter *_adapter);
  void setUrl(const QUrl &_url);
  void setProxy(const QNetworkProxy &_proxy);

public Q_SLOTS:
  void connectToHost();
  void disconnectFromHost();
  void reset(bool _full = true);

private:
  QScopedPointer<JsonRpcClientPrivate> d_ptr;

  void processData(const QJsonObject &_obj);
  QJsonObject createNewObject();

private Q_SLOTS:
  void sendRequest();
  void readyRead();
  void resetRequest();

  void error(QAbstractSocket::SocketError _error);
  void jsonRpcMethod(const QString &_method);
  void jsonRpcMethod(const QString &_method, QVariantMap _args);
  void jsonRpcMethod(const QString &_method, QVariantList _args);

  void bytesWritten(qint64 _bytes);

Q_SIGNALS:
  void connected();
  void socketErrorSignal(QAbstractSocket::SocketError _error, const QString &_message);

};

}
