#pragma once

#include <QObject>

namespace MinerGate {

class SignalBlocker {
public:
  explicit SignalBlocker(QObject* _object):
    m_obj(_object) {
    m_obj->blockSignals(true);
  }
  ~SignalBlocker() {
    m_obj->blockSignals(false);
  }
private:
  QObject* m_obj;
};

}
