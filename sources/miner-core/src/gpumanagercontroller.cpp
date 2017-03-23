#include "miner-core/gpumanagercontroller.h"
#include "cudaminer/cudamanager.h"
#include "openclminer/openclmanager.h"
#include "utils/familyinfo.h"

namespace MinerGate {

GPUManagerPtr cudaManager;
GPUManagerPtr oclManager;

GPUManagerPtr GPUManagerController::get(ImplType implType) {
  switch (implType) {
  case ImplType::CUDA:
    if (!cudaManager)
      cudaManager.reset(new CudaManager);
    return cudaManager;
  case ImplType::OpenCL:
    if (!oclManager)
      oclManager.reset(new OpenCLManager);
    return oclManager;
  }
  Q_ASSERT(false);
  return GPUManagerPtr();
}

quint8 GPUManagerController::deviceCount() {
  quint8 count = 0;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &manager = get(type);
    if (manager)
      count += manager->deviceCount();
  }
  return count;
}

quint8 GPUManagerController::deviceCountForFamiliy(HashFamily _family) {
  quint8 count = 0;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &manager = get(type);
    if (manager && manager->availableForFamily(_family)) {
      for (int deviceNo = 0; deviceNo < manager->deviceCount(); deviceNo++) {
        if (manager->checkDeviceAvailable(deviceNo, _family))
          count++;
      }
    }
  }
  return count;
}

QStringList GPUManagerController::allAvailableDeviceKeys() {
  QStringList result;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &manager = get(type);
    if (manager)
      for (int i = 0; i < manager->deviceCount(); i++)
        result << manager->deviceKey(i);
  }
  return result;
}

QStringList GPUManagerController::allAvailableDeviceKeys(HashFamily _family) {
  QStringList result;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &manager = get(type);
    if (manager && manager->availableForFamily(_family))
      for (int i = 0; i < manager->deviceCount(); i++)
        if (manager->checkDeviceAvailable(i, _family))
          result << manager->deviceKey(i);
  }
  return result;
}

QMap<QString, QVariant> GPUManagerController::allDevices() {
  QMap<QString, QVariant> devices;
  for (auto type = ImplType::FirstType; type <= ImplType::LastType; ++type) {
    const GPUManagerPtr &manager = get(type);
    if (!manager)
      continue;
    for (int i = 0; i < manager->deviceCount(); i++) {
      const QString &deviceKey = manager->deviceKey(i);
      const QVariant &deviceProps = manager->deviceProps(i);
      if (!devices.contains(deviceKey))
        devices.insert(deviceKey, deviceProps);
    }
  }
  return devices;
}

quint8 GPUManagerController::maxAvailableDeviceCount() {
  quint8 result = 0;
  for (auto family = HashFamily::First; family <= HashFamily::Last; ++family) {
    quint8 countForFamily = deviceCountForFamiliy(family);
    if (countForFamily > result)
      result = countForFamily;
  }
  return result;
}

}
