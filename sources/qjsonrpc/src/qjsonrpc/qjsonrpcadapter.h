
#pragma once

#include <QObject>
#include <QVariant>

#include "qjsonrpccommonprerequisites.h"

namespace JsonRpc {

class JsonRpcAdapter : public QObject {
  Q_OBJECT

public:
  JsonRpcAdapter();
  virtual ~JsonRpcAdapter();

Q_SIGNALS:
  void jsonrpc_error(qint32 _code, QString _message, QByteArray _data, QVariantMap _request);
};

}
