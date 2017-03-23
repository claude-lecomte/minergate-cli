#include "miner-core/poolconfig.h"
#include "miner-core/coinmanager.h"

#include "utils/environment.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace MinerGate {


PoolConfig::PoolConfig() :
  QObject() {
}

QString PoolConfig::casheFileName() {
  return Env::dataDir().absoluteFilePath("pools.config");
}

void PoolConfig::init() {
  // read standard pool-config
  QJsonDocument doc;

  {
    QFile file(casheFileName());
    if (!file.exists())
      file.setFileName(":/other/pool_config");
    file.open(QFile::ReadOnly);
    doc = QJsonDocument::fromJson(file.readAll());
  }
  Q_ASSERT(!doc.isEmpty());
  CoinManager::instance().init(doc.object().toVariantMap());
}

void PoolConfig::saveConfig(const QJsonDocument &_doc) {
  // cashing pool config
  QFile file(casheFileName());
  file.open(QFile::WriteOnly);
  file.write(_doc.toJson());
}

}
