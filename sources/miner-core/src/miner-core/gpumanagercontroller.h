#pragma once

#include "miner-abstract/abstractgpumanager.h"

namespace MinerGate {

class GPUManagerController
{
public:
  static GPUManagerPtr get(ImplType implType);
  static quint8 deviceCount();
  static quint8 deviceCountForFamiliy(HashFamily _family);
  static QStringList allAvailableDeviceKeys();
  static QStringList allAvailableDeviceKeys(HashFamily _family);
  static QMap<QString, QVariant> allDevices();
  static quint8 maxAvailableDeviceCount();
};

}
