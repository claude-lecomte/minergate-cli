#pragma once

#include "qjsonrpc/qjsonrpcadapter.h"

#include <QVariantMap>

class EthStratumAdapter : public JsonRpc::JsonRpcAdapter {
  Q_OBJECT
public:
  EthStratumAdapter() : JsonRpc::JsonRpcAdapter() {}
  ~EthStratumAdapter() {}

Q_SIGNALS:
  void jsonrpc_request_login(QVariantMap _params);
  void jsonrpc_response_login(QVariantList _params);
  void jsonrpc_request_eth_getWork(QVariantMap _params);
  void jsonrpc_response_eth_getWork(QVariantList _params);
  void jsonrpc_request_eth_submitWork(QVariantList _params);
  void jsonrpc_response_eth_submitWork(bool result);
  void jsonrpc_notification_eth_work(QVariantList _params);
};
