#pragma once

#include <QString>
#include <QSharedPointer>
#include <QScopedPointer>
#ifdef Q_OS_MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace MinerGate {

class AlgoContext {
public:
  cl_context context = nullptr;
  cl_command_queue commandQueues = nullptr;
  cl_program program = nullptr;
  void *extraData = nullptr;
  size_t globalWorkSize = 0;
  size_t workgroupSize = 0; // = localWorkSizes
  size_t maxGlobalWorkSize = 0;
  ~AlgoContext() {
    if (program)
      clReleaseProgram(program);
    if (commandQueues)
      clReleaseCommandQueue(commandQueues);
    if (context)
      clReleaseContext(context);
  }
};
typedef QScopedPointer<AlgoContext> AlgoContextPtr;

struct OCLDevice {
  unsigned  platformId = 0;
  QString deviceName;
  double OCLVersion = 0;
  cl_device_id deviceID = nullptr;
  size_t globalMemSize = 0;
  size_t maxMemAllocSize = 0;
  size_t maximumWorkGroupSize = 0;
};

class OCLDevicePtr: public QSharedPointer<OCLDevice> {
public:
  OCLDevicePtr():
    QSharedPointer<OCLDevice>() {
  }

  OCLDevicePtr(OCLDevice *_device):
    QSharedPointer<OCLDevice>(_device) {
  }

  OCLDevicePtr(const OCLDevicePtr &other):
    QSharedPointer<OCLDevice>(other) {
  }
};

}

Q_DECLARE_METATYPE(MinerGate::OCLDevicePtr)
