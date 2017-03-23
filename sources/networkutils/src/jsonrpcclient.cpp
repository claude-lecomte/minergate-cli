
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQueue>

#include "utils/logger.h"

#include "networkutils/jsonrpcclient.h"

namespace MinerGate {

class JsonRpcClientPrivate {
public:
  JsonRpcClientPrivate(const QString &_host, quint16 _port) : m_host(_host), m_port(_port) {}
  QString m_host;
  quint16 m_port;
  QTcpSocket m_sock;
  QQueue<QByteArray> m_send_buffer;
};

JsonRpcClient::JsonRpcClient(const QString &_host, quint16 _port) : QObject(), d_ptr(new JsonRpcClientPrivate(_host, _port)) {
  connect(&d_ptr->m_sock, SIGNAL(connected()), this, SLOT(connectedToHost()));
  connect(&d_ptr->m_sock, SIGNAL(disconnected()), this, SLOT(disconnectedFromHost()));
  connect(&d_ptr->m_sock, SIGNAL(readyRead()), this, SLOT(readyRead()));
  connect(&d_ptr->m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
  connect(&d_ptr->m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(errorSignal(QAbstractSocket::SocketError)));
}

JsonRpcClient::~JsonRpcClient() {
}

void JsonRpcClient::connectToHost(const QString &_host, quint16 _port) {
  d_ptr->m_sock.connectToHost(_host, _port);
}

void JsonRpcClient::send(const QJsonObject &_obj) {
  QByteArray data = QJsonDocument(_obj).toJson(QJsonDocument::Compact);
  if (d_ptr->m_send_buffer.isEmpty() || d_ptr->m_send_buffer.last() != data) {
    d_ptr->m_send_buffer.append(data);
  }

  if (d_ptr->m_sock.state() == QAbstractSocket::UnconnectedState) {
    d_ptr->m_sock.connectToHost(d_ptr->m_host, d_ptr->m_port);
  }
}

void JsonRpcClient::reset() {
  d_ptr->m_send_buffer.clear();
}

void JsonRpcClient::connectedToHost() {
  if (d_ptr->m_send_buffer.size()) {
    d_ptr->m_sock.write(d_ptr->m_send_buffer.dequeue());
  }
}

void JsonRpcClient::disconnectedFromHost() {
}

void JsonRpcClient::readyRead() {
  QByteArray data = d_ptr->m_sock.readAll();
  QJsonDocument json_doc = QJsonDocument::fromJson(data);
  Q_EMIT dataSignal(json_doc.object());
}

void JsonRpcClient::error(QAbstractSocket::SocketError _error) {
  switch (_error) {
  case QAbstractSocket::ConnectionRefusedError:
    if (!d_ptr->m_send_buffer.isEmpty()) {
      d_ptr->m_sock.connectToHost(d_ptr->m_host, d_ptr->m_port);
    }

    break;
  default:
    break;
  }
}

}
