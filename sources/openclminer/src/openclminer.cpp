#include "openclminer/openclminer.h"
#include "openclminer/openclminer_p.h"

namespace MinerGate {

OpenCLMiner::OpenCLMiner(OpenCLMinerPrivate &_d)
  : AbstractGPUMiner(_d)
{}

OpenCLMiner::~OpenCLMiner() {
}

quint32 OpenCLMiner::intensity2deviceFactor(quint8 _intensity) const {
  return std::pow(2, (4 - _intensity));
}

quint8 OpenCLMiner::deviceFactor2Intensity(quint32 _deviceFactor) const {
  return 4 - std::log2(_deviceFactor);
}

}


