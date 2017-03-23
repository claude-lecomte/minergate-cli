#pragma once

#include <QScopedPointer>

#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate {

class GPUTesterPrivate;
class GPUTester {
  Q_DECLARE_PRIVATE(GPUTester)
  Q_DISABLE_COPY(GPUTester)
public:
  ~GPUTester();
  GPUTester(ImplType _type, HashFamily _family, quint32 _deviceNo, quint32 _firstParam, quint32 _secondParam);
  bool run(quint32 &result, QString &_error);
  static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
private:
  QScopedPointer<GPUTesterPrivate> d_ptr;
};

}
