#pragma once

#include <QScopedPointer>

class QVariant;

namespace MinerGate {

class GpuTesterPrivate;
class GpuTester {
  Q_DECLARE_PRIVATE(GpuTester)
public:
  enum Error {NoError};
  GpuTester();
  void run();
  ~GpuTester();
  static void stop();
private:
  QScopedPointer<GpuTesterPrivate> d_ptr;
};

}
