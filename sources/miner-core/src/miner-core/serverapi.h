
#pragma once

#include <QObject>
#include <QThread>
#include <QJsonArray>

#include "minercorecommonprerequisites.h"

namespace MinerGate {

class ServerApiStatePrivate;

const QString SERVER_WS("https://wsg.minergate.com");
const QString SERVER_API("https://api.minergate.com");
const QString SERVER_WEB("https://minergate.com");

const QString KEY_SERVER_WEB("web-server");
const QString KEY_SERVER_API("api-server");
const QString KEY_SERVER_WS("ws-server");

class ServerApiState {
  Q_DECLARE_PRIVATE(ServerApiState)
public:
  ServerApiState();
  ~ServerApiState();

  AuthState authState() const;
  bool totpRequired() const;

  void setAuthState(AuthState _state);
  void setTotpRequired(bool _on);

private:
  QScopedPointer<ServerApiStatePrivate> d_ptr;
};

class TransactionParser : public QThread {
  Q_OBJECT

public:
  TransactionParser();
  ~TransactionParser();

  void setData(const QByteArray &_data);
  void setData(const QJsonArray &_data);

protected:
  void run();

private:
  QJsonArray m_data;

Q_SIGNALS:
  void newTransactionSignal(const QJsonObject &_payout_obj);
};

class ServerApiPrivate;
class TestHistory;
class BenchmarkData;
class ServerApi : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(ServerApi)
  Q_DECLARE_PRIVATE(ServerApi)
public:
  static ServerApi &instance() {
    static ServerApi inst;
    return inst;
  }

  ~ServerApi();

  bool init();
  void abort();

  const ServerApiState &state() const;
private:
  QScopedPointer<ServerApiPrivate> d_ptr;

  ServerApi();
  QByteArray bcrypt(const QByteArray &_pass, const QByteArray &_salt) const;
  Error processError(quint32 _code, const QByteArray &_data, const QString& _eventCategory = QString());

private Q_SLOTS:
  void poolResp(quint32 _code, const QByteArray &_data, const QVariant &_user_data);
Q_SIGNALS:
  void loggedInSignal();
  void loggedOutSignal();
  void pleaseLogin();
  void errorSignal(Error _err);
  void connectOk();
  void updatesAvailableSignal(const QUrl &_url);
  void criticalUpdatesAvailableSignal(const QUrl &_url);
  void stateChangedSignal();
  void newPayoutSignal(const QJsonObject &_payoutObj);
  void newInvoiceSignal(const QJsonObject &_invoiceObj);
  void payoutError(Error _err);
  void profitDataUpdatedSignal(const QStringList& _currency_list);
  void benchmarkResults(const QString &serverText, quint8 stars, const QString &link);
  void benchmarkHistoryResults(const TestHistory &history);
  void startingBenchmarkRequired();
  void serverTimeSignal(const QDateTime &time);
  void invoiceOperationFinished();
  void invoiceTotpRequired(const QString &token);
  void invoiceNotEnoughMoney(const QString &token);
};

}
