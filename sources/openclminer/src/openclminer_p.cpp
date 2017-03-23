#include "openclminer/openclminer_p.h"

namespace MinerGate {

bool OpenCLMinerPrivate::init() {
  return true;
}

OpenCLMinerPrivate::OpenCLMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
  AbstractGPUMinerPrivate(_device, _manager) {
}

OpenCLMinerPrivate::~OpenCLMinerPrivate() {
}

}
