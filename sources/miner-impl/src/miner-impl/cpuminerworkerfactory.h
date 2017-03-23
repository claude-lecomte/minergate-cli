#pragma once

#include <atomic>
#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate {

enum class CPUWorkerType : quint8;
class CPUMinerWorkerFactory
{
public:
  static CPUMinerWorkerPtr create(CPUWorkerType _type, volatile std::atomic<quint32> &_hash_counter,
                                  bool _fee, QObject* _parent, const QVariant &_options);
};

}
