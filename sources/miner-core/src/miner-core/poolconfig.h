#pragma once
#include <QObject>

namespace MinerGate {

class PoolConfig : public QObject {
  Q_OBJECT
public:
  static PoolConfig &instance() {
    static PoolConfig m_instance;
    return m_instance;
  }
  void init();
  void saveConfig(const QJsonDocument &_doc);
private:
  PoolConfig();
  static QString casheFileName();
};

}
