#pragma once

#include "qjsonrpc/qjsonrpcadapter.h"

#include <QVariantMap>

class StratumAdapter : public JsonRpc::JsonRpcAdapter {
  Q_OBJECT
public:
  StratumAdapter() : JsonRpc::JsonRpcAdapter() {}
  ~StratumAdapter() {}

Q_SIGNALS:
  void jsonrpc_request_login(QVariantMap _params);
  void jsonrpc_response_login(QVariantMap _params);
  void jsonrpc_request_getjob(QVariantMap _params);
  void jsonrpc_response_getjob(QVariantMap _params);
  void jsonrpc_request_submit(QVariantMap _params);
  void jsonrpc_response_submit(QVariantMap _params);
  void jsonrpc_notification_job(QVariantMap _params);
};
