
#include <QUrl>
#include <QMetaMethod>
#include <QTcpSocket>
#include <QSharedPointer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QQueue>
#include <QTimer>

#include "qjsonrpc/qjsonrpcclient.h"
#include "qjsonrpc/qjsonrpcadapter.h"

namespace JsonRpc {

typedef QSharedPointer<JsonRpcMethodProxy> JsonRpcMethodProxyPtr;

JsonRpcMethodProxy::JsonRpcMethodProxy(QObject *_adapter, const QMetaMethod &_method) : QObject(), m_method(_method), m_rpc_method() {
  m_rpc_method = QString::fromUtf8(m_method.name()).section("jsonrpc_request_", 0, 0, QString::SectionSkipEmpty);
  switch(m_method.parameterCount()) {
  case 0:
    connect(_adapter, QString(SIGNAL(%1)).arg(QString::fromUtf8(m_method.methodSignature())).toUtf8(),
      SLOT(method()), Qt::QueuedConnection);
    break;
  case 1:
    connect(_adapter, QString(SIGNAL(%1)).arg(QString::fromUtf8(m_method.methodSignature())).toUtf8(),
      QString(SLOT(method(%1))).arg(QMetaType::typeName(m_method.parameterType(0))).toUtf8(), Qt::QueuedConnection);
    break;
  }
}

JsonRpcMethodProxy::~JsonRpcMethodProxy() {
}

void JsonRpcMethodProxy::method() {
  Q_EMIT methodSignal(m_rpc_method);
}

void JsonRpcMethodProxy::method(const QVariantMap &_args) {
  Q_EMIT methodSignal(m_rpc_method, _args);
}

void JsonRpcMethodProxy::method(const QVariantList &_args) {
  Q_EMIT methodSignal(m_rpc_method, _args);
}

class JsonRpcQueueHandlerPrivate {
public:
  JsonRpcQueueHandlerPrivate() : m_req_queue(), m_req_in_progress() {
  }

  QQueue<QJsonObject> m_req_queue;
  QMap<quint64, QJsonObject> m_req_in_progress;
};

JsonRpcQueueHandler::JsonRpcQueueHandler() : QObject(), d_ptr(new JsonRpcQueueHandlerPrivate) {
}

JsonRpcQueueHandler::~JsonRpcQueueHandler() {
}

void JsonRpcQueueHandler::addRequest(const QJsonObject &_obj) {
  d_ptr->m_req_queue.append(_obj);
}

QJsonObject JsonRpcQueueHandler::getRequestForProcess() {
  QJsonObject obj;
  if (!d_ptr->m_req_queue.isEmpty()) {
    obj = d_ptr->m_req_queue.dequeue();
    d_ptr->m_req_in_progress.insert(obj.value("id").toString().toULongLong(), obj);
  }

  return obj;
}

QJsonObject JsonRpcQueueHandler::removeRequest(quint64 _id) {
  return d_ptr->m_req_in_progress.take(_id);
}

void JsonRpcQueueHandler::clear(bool _full) {
  d_ptr->m_req_queue.clear();
  if (_full) {
    d_ptr->m_req_in_progress.clear();
  }
}


class JsonRpcClientPrivate {
public:
  JsonRpcClientPrivate(const QUrl &_url, Version _version) : m_url(_url), m_version(_version), m_id_counter(0),
    m_bytes_to_write(0) {
    m_request_timer.setSingleShot(true);
    m_request_timer.setInterval(10000);
    m_connect_timer.setInterval(3000);
  }

  QUrl m_url;
  Version m_version;
  QTcpSocket m_sock;
  quint64 m_id_counter;
  JsonRpcQueueHandler m_request_queue;
  QTimer m_request_timer;
  QTimer m_connect_timer;
  JsonRpcAdapter *m_adapter;
  QList<JsonRpcMethodProxyPtr> m_methods;
  QMap<QString, QMetaMethod> m_requests;
  QMap<QString, QMetaMethod> m_responces;
  QMap<QString, QMetaMethod> m_notifications;
  QMetaMethod m_error;
  qint64 m_bytes_to_write;
};

JsonRpcClient::JsonRpcClient(const QUrl &_url, Version _version) : QObject(),
  d_ptr(new JsonRpcClientPrivate(_url, _version)) {
  d_ptr->m_sock.setSocketOption(QAbstractSocket::LowDelayOption, 1);
  connect(&d_ptr->m_sock, &QAbstractSocket::connected, this, &JsonRpcClient::connected);
  connect(&d_ptr->m_sock, &QAbstractSocket::connected, this, &JsonRpcClient::sendRequest);
  connect(&d_ptr->m_sock, &QIODevice::readyRead, this, &JsonRpcClient::readyRead);
  connect(&d_ptr->m_sock, &QIODevice::bytesWritten, this, &JsonRpcClient::bytesWritten);
  connect(&d_ptr->m_sock, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(error(QAbstractSocket::SocketError)));
  connect(&d_ptr->m_request_timer, &QTimer::timeout, this, &JsonRpcClient::resetRequest);
  connect(&d_ptr->m_connect_timer, &QTimer::timeout, this, &JsonRpcClient::connectToHost);
  connect(&d_ptr->m_sock, &QAbstractSocket::connected, &d_ptr->m_connect_timer, &QTimer::stop);
}

JsonRpcClient::~JsonRpcClient() {
}

void JsonRpcClient::connectToHost() {
  d_ptr->m_bytes_to_write = 0;
  if (d_ptr->m_connect_timer.isActive() && sender() != &d_ptr->m_connect_timer) {
    return;
  }

  if (d_ptr->m_sock.state() == QAbstractSocket::ConnectedState) {
    return;
  } else if (d_ptr->m_sock.state() == QAbstractSocket::ConnectingState) {
    d_ptr->m_sock.abort();
    d_ptr->m_sock.close();
  }

#ifdef QT_DEBUG
  qDebug() << "Trying to connect... " << d_ptr->m_url.toString();
#endif

  d_ptr->m_sock.connectToHost(d_ptr->m_url.host(), d_ptr->m_url.port());
}

void JsonRpcClient::disconnectFromHost() {
  d_ptr->m_connect_timer.stop();
  d_ptr->m_sock.abort();
  d_ptr->m_sock.close();
  reset(true);
}

void JsonRpcClient::reset(bool _full) {
  d_ptr->m_request_queue.clear(_full);
}

QUrl JsonRpcClient::url() const {
  return d_ptr->m_url;
}

void JsonRpcClient::setAdapter(JsonRpcAdapter *_adapter) {
  d_ptr->m_adapter = _adapter;
  const QMetaObject *mo = _adapter->metaObject();

  for (quint32 i = mo->methodOffset() - 1; i < mo->methodCount(); ++i) {
    QMetaMethod mm = mo->method(i);
    if (mm.methodType() != QMetaMethod::Signal) {
      continue;
    }

    QString meth_name = mm.name();
    if (meth_name.startsWith("jsonrpc_request_")) {
      JsonRpcMethodProxyPtr proxy(new JsonRpcMethodProxy(_adapter, mm));
      d_ptr->m_methods << proxy;
      connect(proxy.data(), SIGNAL(methodSignal(QString)), SLOT(jsonRpcMethod(QString)), Qt::QueuedConnection);
      connect(proxy.data(), SIGNAL(methodSignal(QString, QVariantMap)), SLOT(jsonRpcMethod(QString, QVariantMap)), Qt::QueuedConnection);
      connect(proxy.data(), SIGNAL(methodSignal(QString, QVariantList)), SLOT(jsonRpcMethod(QString, QVariantList)), Qt::QueuedConnection);
    } else if (meth_name.startsWith("jsonrpc_response_")) {
      QString rpc_math_name = meth_name.section("jsonrpc_response_", 0, 0, QString::SectionSkipEmpty);
      d_ptr->m_responces.insert(rpc_math_name, mm);
    } else if (meth_name.startsWith("jsonrpc_notification_")) {
      QString rpc_math_name = meth_name.section("jsonrpc_notification_", 0, 0, QString::SectionSkipEmpty);
      d_ptr->m_notifications.insert(rpc_math_name, mm);
    } else if(!meth_name.compare("jsonrpc_error")) {
      d_ptr->m_error = mm;
    }
  }
}

void JsonRpcClient::setUrl(const QUrl &_url) {
  d_ptr->m_url = _url;
}

void JsonRpcClient::setProxy(const QNetworkProxy &_proxy) {
  disconnectFromHost();
  d_ptr->m_sock.waitForDisconnected(3000);
  d_ptr->m_sock.setProxy(_proxy);
  connectToHost();
}

void JsonRpcClient::sendRequest() {
  if (d_ptr->m_connect_timer.isActive()) {
    return;
  }

  if (d_ptr->m_sock.state() != QAbstractSocket::ConnectedState) {
    connectToHost();
    return;
  }

  if (d_ptr->m_bytes_to_write > 0) {
#ifdef QT_DEBUG
    qDebug() << "Client is busy. Waiting...";
#endif
    return;
  }

  QJsonObject obj = d_ptr->m_request_queue.getRequestForProcess();
  if (obj.isEmpty()) {
    return;
  }

  QByteArray msg = QJsonDocument(obj).toJson(QJsonDocument::Compact).append("\n\n");
#ifdef QT_DEBUG
  qDebug() << "SEND: " << msg;
#endif
  d_ptr->m_bytes_to_write = msg.size();
  d_ptr->m_sock.write(msg);
  d_ptr->m_request_timer.start();
}

void JsonRpcClient::processData(const QJsonObject &_obj) {
  if (_obj.contains("error") && !_obj.value("error").isNull()) {
    // Error
    QJsonObject req_obj;
    if (_obj.contains("id")) {
      req_obj = d_ptr->m_request_queue.removeRequest(_obj.value("id").toString().toULongLong());
    }

    d_ptr->m_error.invoke(d_ptr->m_adapter, Q_ARG(qint32, _obj.value("error").toObject().value("code").toInt()),
      Q_ARG(QString, _obj.value("error").toObject().value("message").toString()),
      Q_ARG(QByteArray, _obj.value("error").toObject().value("data").toString().toUtf8()),
      Q_ARG(QVariantMap, req_obj.toVariantMap()));
  } else if (!_obj.contains("id")) {
    // Notification
    QString method = _obj.value("method").toString();
    QMetaMethod mm = d_ptr->m_notifications.value(method);
    QVariant params;
    if (_obj.contains("params"))
      params = _obj.value("params").toVariant();
    else if (_obj.contains("result"))
      params = _obj.value("result").toVariant();
    switch(params.type()) {
    case QMetaType::QVariantList:
      mm.invoke(d_ptr->m_adapter, Q_ARG(QVariantList, params.toList()));
      break;
    case QMetaType::QVariantMap:
      mm.invoke(d_ptr->m_adapter, Q_ARG(QVariantMap, params.toMap()));
      break;
    default:
#ifdef QT_DEBUG
      qDebug() << "Unknown params type";
#endif
      return;
    }
  } else {
    d_ptr->m_request_timer.stop();
    quint64 id = _obj.value("id").toString().toULongLong();

    QJsonObject req_obj = d_ptr->m_request_queue.removeRequest(id);
    QString method = req_obj.value("method").toString();

    if (_obj.contains("result") || _obj.contains("params")) {
      // Response
      if (!d_ptr->m_responces.contains(method)) {
#ifdef QT_DEBUG
        qDebug() << "Response method not found";
#endif
        return;
      }

      QMetaMethod mm = d_ptr->m_responces.value(method);
      QVariant result;
      if (_obj.contains("result"))
        result = _obj.value("result").toVariant();
      else
        result = _obj.value("params").toVariant();

      switch (result.type()) {
      case QMetaType::QVariantList:
        mm.invoke(d_ptr->m_adapter, Q_ARG(QVariantList, result.toList()));
        break;
      case QMetaType::QVariantMap:
        mm.invoke(d_ptr->m_adapter, Q_ARG(QVariantMap, result.toMap()));
        break;
      case QMetaType::Bool:
        mm.invoke(d_ptr->m_adapter, Q_ARG(bool, result.toBool()));
        break;
      default:
#ifdef QT_DEBUG
        qDebug() << "Unknown result type";
#endif
        return;
      }
    }
  }
}

void JsonRpcClient::readyRead() {
  QTextStream str(&d_ptr->m_sock);
  QString json_str;
  while (!str.atEnd()) {
    json_str = str.readLine();
#ifdef QT_DEBUG
    qDebug() << "RECV: " << json_str;
#endif
    processData(QJsonDocument::fromJson(json_str.toUtf8()).object());
  }

  sendRequest();
}

void JsonRpcClient::resetRequest() {
  d_ptr->m_sock.abort();
  reset();
  connectToHost();
}

void JsonRpcClient::error(QAbstractSocket::SocketError _error) {
  reset(true);
  d_ptr->m_request_timer.stop();
  Q_EMIT socketErrorSignal(_error, d_ptr->m_sock.errorString());
  if (!d_ptr->m_connect_timer.isActive()) {
    d_ptr->m_connect_timer.start();
  }
}


void JsonRpcClient::jsonRpcMethod(const QString &_method) {
  QJsonObject obj = createNewObject();

  obj.insert("method", _method);
  d_ptr->m_request_queue.addRequest(obj);
  sendRequest();
}

void JsonRpcClient::jsonRpcMethod(const QString &_method, QVariantMap _args) {
  QJsonObject obj = createNewObject();

  obj.insert("method", _method);
  obj.insert("params", QJsonDocument::fromVariant(_args).object());
  d_ptr->m_request_queue.addRequest(obj);
  sendRequest();
}

void JsonRpcClient::jsonRpcMethod(const QString &_method, QVariantList _args) {
  QJsonObject obj = createNewObject();

  obj.insert("method", _method);
  obj.insert("params", QJsonDocument::fromVariant(_args).array());
  d_ptr->m_request_queue.addRequest(obj);
  sendRequest();
}

QJsonObject JsonRpcClient::createNewObject() {
  QJsonObject obj;
  switch (d_ptr->m_version) {
  case Version::JsonRpc20:
    obj.insert("jsonrpc", QString("2.0"));
    break;
  default:
    break;
  }

  obj.insert("id", QString::number(++d_ptr->m_id_counter));
  return obj;
}

void JsonRpcClient::bytesWritten(qint64 _bytes) {
  d_ptr->m_bytes_to_write -= _bytes;
  if (d_ptr->m_bytes_to_write == 0) {
    sendRequest();
  }
}

}
