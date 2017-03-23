#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtCore/qmath.h>
#include <QTimer>
#include <QThread>
#include <QPair>
#include <QCoreApplication>
#include <QProcess>

#include "utils/environment.h"
#include "utils/logger.h"
#include "utils/settings.h"

#include "miner-core/serverapi.h"
#include "miner-core/coin.h"
#include "miner-core/coinmanager.h"
#include "miner-core/poolconfig.h"

#include "networkutils/downloadmanager.h"
#include "networkutils/networkenvironment.h"

#include "ow-crypt.h"

namespace MinerGate {

const int TEST_DELAY(10000);
const QString APPLICATION_JSON("application/json");

const int WITHDRAWAL_UPDATE_TIMEOUT(60000);
const int INVOICES_UPDATE_TIMEOUT(60000);
const quint32 POOL_REQUEST_INTERVAL(3600000);

// HTTP api paths
const QString P_UPDATES("/api/2.2/client/update");
const QString P_LOGIN("/api/2.2/auth/login");
const QString P_REGISTER("/api/2.2/auth/register");
const QString P_WITHDRAWALS("/api/2.3/user/withdrawals");
const QString P_WITHDRAW("/api/2.2/user/withdraw");
const QString P_CONFIG("/api/2.4/pool/config");
const QString P_SOCKETIO("/sio/1/");
const QString P_BENCHMARK("/api/2.3/benchmark");
const QString P_STARTING_BENCHMARK("/api/2.3/benchmark/showIntro");
const QString P_TIME("/api/2.3/time");
const QString P_ACHIEVEMENTS_COMPLETED("/api/2.3/achievementsCompleted");
const QString P_INVOICES("/api/2.4/invoices");
const QString P_APPROVE_INVOICE("/api/2.3/invoice/approve");
const QString P_REJECT_INVOICE("/api/2.3/invoice/reject");

// HTTP api headers
const QString H_EMAIL("email");
const QString H_PASSWORD("password");
const QString H_TOTP("totp");
const QByteArray H_TOKEN("token");

// HTTP api errors
const QString E_WRONGEMAILORPASS("WrongEmailOrPassword");
const QString E_TOTPREQUIRED("TotpRequired");
const QString E_WRONGTOTP("WrongTotp");
const QString E_MISSEMAIL("MissingEmail");
const QString E_MISSPASS("MissingPassword");
const QString E_MISSINGTOKEN("MissingToken");
const QString E_INVALIDTOKEN("TokenInvalid");
const QString E_TOKENEXPIRED("TokenExpired");
const QString E_MISSINGARGUMENT("MissingArgument");
const QString E_INVALIDPAYMENTID("InvalidPaymentId");
const QString E_PAYMENTIDREQUIRED("PaymentIdRequired");
const QString E_WITHDRAWALSDISABLEDFORUSER("WithdrawalsDisabledForUser");
const QString E_INVALIDADDRESS("InvalidAddress");
const QString E_USERALREADYEXISTS("UserAlreadyExists");
const QString E_INVALIDEMAIL("InvalidEmail");
const QString E_NOTENOUGHMONEY("NotEnoughMoney");
const QString E_LOGINMAXCOUNTEXCEEDED("LoginMaxCountExceeded");
const QString E_AMOUNT_BELOW_LIMIT("AmountBelowLimit");

//HTTP api params
const QString PAR_STATS_POOL_BALANCE("poolBalance");
const QString PAR_STATS_SHARES("shares");
const QString PAR_STATS_GOOD_SHARES("good");
const QString PAR_STATS_BAD_SHARES("bad");
const QString PAR_STATS_EQ_SHARES("goodEq");
const QString PAR_STATS_BAD_EQ_SHARES("badEq");
const QString PAR_REWARD_METHOD("paymentModels");
const QString PAR_WORKERS_COUNT("minersCount");
const QString PAR_TEXT("text");
const QString PAR_STARS("stars");
const QString PAR_LINK("link");
const QString PAR_SHOW("show");

// settings keys
const QString KEY_API_TOKEN("apiToken");
const QString KEY_EMAIL("email");

// other consts
const QString BEARER("Bearer %1");

const QString WS_NAME("name");
const QString WS_ARGS("args");

const QString WS_CMD_STATS("channel:mining.stats.changes");
const QString WS_CMD_PROFILE("channel:user.profile");
const QString WS_CMD_RATING("channel:cc.rating");
const QString WS_CMD_USER_BALANCES("channel:user.balance");
const QString WS_CMD_AUTH("auth");
const QString WS_CMD_WORKER_STATUSES("workers.statuses");
const QString WS_CMD_INVOICES("invoices");

class ServerApiStatePrivate {
public:
  ServerApiStatePrivate() : m_auth_state(AuthState::LoggedOut), m_totp_required(false)  {
  }

  AuthState m_auth_state;
  bool m_totp_required;
};

ServerApiState::ServerApiState() : d_ptr(new ServerApiStatePrivate) {
}

ServerApiState::~ServerApiState() {
}

AuthState ServerApiState::authState() const {
  Q_D(const ServerApiState);
  return d->m_auth_state;
}

bool ServerApiState::totpRequired() const {
  Q_D(const ServerApiState);
  return d->m_totp_required;
}

void ServerApiState::setAuthState(AuthState _state) {
  Q_D(ServerApiState);
  d->m_auth_state = _state;
}

void ServerApiState::setTotpRequired(bool _on) {
  Q_D(ServerApiState);
  d->m_totp_required = _on;
}

TransactionParser::TransactionParser() : QThread(), m_data() {
}

TransactionParser::~TransactionParser() {
}

void TransactionParser::setData(const QByteArray &_data) {
  m_data = QJsonDocument::fromJson(_data).array();

}

void TransactionParser::setData(const QJsonArray &_data) {
  m_data = _data;
}

void TransactionParser::run() {
  Q_FOREACH (const QJsonValue &_val, m_data) {
    QJsonObject obj = _val.toObject();
    if (!obj.isEmpty())
      Q_EMIT newTransactionSignal(obj);
  }
}

class ServerApiPrivate {
public:
  ServerApiPrivate() {
  }

  DownloadManagerPtr m_pool_man;
  ServerApiState m_state;
};

ServerApi::ServerApi() : QObject(), d_ptr(new ServerApiPrivate()) {
  Q_D(ServerApi);
  const QUrl &apiServer = Settings::instance().readParam(KEY_SERVER_API, SERVER_API).value<QUrl>();
  Settings::instance().writeParam(KEY_SERVER_API, apiServer.toString());
}

ServerApi::~ServerApi() {
}

bool ServerApi::init() {
  Q_D(ServerApi);
  QUrl poolUrl(Settings::instance().readParam(KEY_SERVER_API, SERVER_API).toUrl());
  poolUrl.setPath(QString(P_CONFIG));

  QNetworkRequest poolReq(poolUrl);
  poolReq.setHeader(QNetworkRequest::ContentTypeHeader, APPLICATION_JSON);
  d->m_pool_man.reset(new DownloadManager(poolReq, "pool", POOL_REQUEST_INTERVAL));
  connect(d->m_pool_man.data(), &DownloadManager::finishedSignal, this, &ServerApi::poolResp);

  QTimer pool_timer;
  pool_timer.start(10000);
  QEventLoop loop;

  QMetaObject::Connection finished_conn = connect(d->m_pool_man.data(), &DownloadManager::finishedSignal,
                                                  [&loop](quint32 _code) {
      loop.exit(_code == 200 ? 0 : 1);
});

  bool stop = false;
  qint32 loop_result = 0;
  connect(d->m_pool_man.data(), &DownloadManager::abortSignal, &loop, [&loop, &stop, &loop_result]() {
    stop = true;
    loop_result = 1;
    loop.exit(1);
    qApp->processEvents();
  });

  connect(&pool_timer, &QTimer::timeout, [&loop]() { loop.exit(1); });
  quint32 replies = 0;
  do {
    if (loop_result == 1) {
      QThread::sleep(1);
    }

    d->m_pool_man->get();
    Logger::info(QString(), "Pool parameters query...");
    if (++replies == 5) {
      loop_result = 1;
      break;
    }
    if (stop)
      break;
  }
  while((loop_result = loop.exec()) == 1);

  disconnect(finished_conn);
  if (loop_result) {
    Logger::critical(QString(), QString(tr("Unable to get configuration from %1.")).arg(Settings::instance().readParam(KEY_SERVER_API, SERVER_API).toString()));
    return false;
  }

  if (Env::consoleApp())
    return true;
  return true;
}

const ServerApiState &ServerApi::state() const {
  Q_D(const ServerApi);
  return d->m_state;
}

QByteArray ServerApi::bcrypt(const QByteArray &_pass, const QByteArray &_salt) const {
  char *crypted_pass = 0;
  qint32 pass_size;
  crypt_ra(_pass.data(), _salt.data(), reinterpret_cast<void**>(&crypted_pass), &pass_size);
  QByteArray res(crypted_pass);
  return res;
}

Error ServerApi::processError(quint32 _code, const QByteArray &_data, const QString& _eventCategory) {
  Q_D(ServerApi);
  if (_code == 200) {
    return Error::NoError;
  }

  if (!_code) {
    qCritical() << "network error" << _data;
    return Error::NetworkProblem;
  }

  QJsonDocument doc = QJsonDocument::fromJson(_data);
  if (doc.isNull()) {
    qCritical("JSON error: %s", _data.data());
    return Error::UnknownError;
  }

  QVariantMap data_map = doc.object().toVariantMap();
  QString error = data_map.value("error", QString()).value<QString>();
  QString message = data_map.value("message", QString()).value<QString>();
  Logger::critical(QString(), QString("API auth error. Error: %1  Message: %2").arg(error).arg(message));

  if (!error.compare(E_WRONGEMAILORPASS)) {
    return Error::InvalidEmailOrPass;
  } else if (!error.compare(E_TOTPREQUIRED)) {
    return Error::TotpRequired;
  } else if (!error.compare(E_WRONGTOTP)) {
    return Error::WrongTotp;
  } else if (!error.compare(E_MISSINGARGUMENT)) {
    return Error::MissingArgument;
  } else if (!error.compare(E_INVALIDPAYMENTID)) {
    return Error::InvalidPaymentId;
  } else if (!error.compare(E_PAYMENTIDREQUIRED)) {
    return Error::PaymentIdRequired;
  } else if (!error.compare(E_WITHDRAWALSDISABLEDFORUSER)) {
    return Error::WithdrawalsDisabledForUser;
  } else if (!error.compare(E_INVALIDADDRESS)) {
    return Error::InvalidAddress;
  } else if (!error.compare(E_USERALREADYEXISTS)) {
    return Error::UserAlreadyExists;
  } else if (!error.compare(E_INVALIDEMAIL)) {
    return Error::InvalidEmail;
  } else if (!error.compare(E_LOGINMAXCOUNTEXCEEDED)) {
    return Error::LoginMaxCountExceeded;
  } else if (!error.compare(E_AMOUNT_BELOW_LIMIT)) {
    return Error::AmountBelowLimit;
  } else if (!error.compare(E_NOTENOUGHMONEY)) {
    return Error::NotEnoughMoney;
  } else if (!error.compare(E_INVALIDTOKEN)) {
    // clear token and restart application
    Logger::warning(QString::null, "invalid token");
    Settings::instance().writeParam(KEY_API_TOKEN, QString());
    Settings::instance().sync();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    qApp->quit();
    return Error::TokenInvalid;
  } else if (!error.compare(E_TOKENEXPIRED)) {
    Settings::instance().writeParam(KEY_API_TOKEN, QString());
    d->m_state.setAuthState(AuthState::TokenExpired);
    Q_EMIT stateChangedSignal();
  } else
    return Error::NetworkProblem;

  qCritical("Error: %s", error.toLocal8Bit().data());
  return Error::UnknownError;
}

void ServerApi::poolResp(quint32 _code, const QByteArray &_data, const QVariant &_user_data) {
  Q_D(ServerApi);
  Error err = processError(_code, _data);
  if (_data.isEmpty() || err != Error::NoError) {
    Logger::critical(QString(), QString("POOL response data: %1").arg(QString::fromUtf8(_data)));
    Q_EMIT errorSignal(err);
    d->m_pool_man->setInterval(5000);
    return;
  }

#ifdef DEEP_DEBUG
  Logger::debug(QString(), QString("POOL response %1").arg(_data.data()));
#endif

  const QJsonDocument &jsonDoc = QJsonDocument::fromJson(_data);
  const QVariantMap &data_map = jsonDoc.object().toVariantMap();
  if (data_map.isEmpty()) {
    Logger::critical(QString(), QString("POOL empty response"));
    return;
  }

  PoolConfig::instance().saveConfig(jsonDoc);

  QVariantMap dev_fee_map = data_map.value("devFeeCC").toMap();
  if (!dev_fee_map.isEmpty()) {
    QString pool_ip = dev_fee_map.value("ip").toString();
    quint16 pool_stratum_port = dev_fee_map.value("stratumPort").toUInt();
    CoinManager::instance().setDevFeePool(QUrl::fromUserInput(QString("stratum+tcp://%1:%2").arg(pool_ip).arg(pool_stratum_port)));
  }

  if (CoinManager::instance().miners().isEmpty()) {
    Logger::warning(QString::null, "no standard config loaded!");
    CoinManager::instance().init(data_map);
  } else {
    for (auto it = data_map.constBegin(); it != data_map.constEnd(); ++it) {
      QString coin_key = it.key();
      QVariantMap coin_map = it.value().toMap();
      QVariantMap pool_map = coin_map.value("server").toMap();
      QString pool_ip = pool_map.value("ip").toString();
      quint16 pool_stratum_port = pool_map.value("stratumPort").toUInt();

      QVariantMap mm_map = pool_map.value("merged").toMap();
      QVariantMap miner_params;
      if (!mm_map.isEmpty()) {
        for (QVariantMap::const_iterator it = mm_map.begin(); it != mm_map.end(); ++it) {
          QString ip = it.value().toMap().value("ip").toString();
          quint16 port = it.value().toMap().value("stratumPort").value<quint16>();
          miner_params.insert(QString("mergedPools/%1").arg(it.key()), QString("stratum+tcp://%1:%2").arg(ip).arg(port));
        }
      }

      miner_params.insert("engine", coin_key);
      miner_params.insert("pool", QString("stratum+tcp://%1:%2").arg(pool_ip).arg(pool_stratum_port));
      Settings::instance().addMiner(miner_params);
    }
  }

  if (d->m_pool_man->interval() != POOL_REQUEST_INTERVAL) {
    d->m_pool_man->setInterval(POOL_REQUEST_INTERVAL);
    Q_EMIT connectOk();
  }
}

void ServerApi::abort() {
  Q_D(ServerApi);
  if (!d->m_pool_man.isNull())
    d->m_pool_man->abort();
}

}
