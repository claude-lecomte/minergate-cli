#pragma once

#include <QSharedPointer>

namespace MinerGate {

class AbstractCPUMinerWorker;
#ifndef Q_OS_MAC
class AbstractGPUMiner;
class GPUMinerWorker;
#endif
enum class HashFamily: quint8;

#ifndef Q_OS_MAC
typedef QSharedPointer<AbstractGPUMiner> GPUCoinPtr;
typedef QHash<HashFamily, GPUCoinPtr> GPUMinerMap;
typedef QSharedPointer<GPUMinerWorker> GPUMinerWorkerPtr;
#endif
typedef QSharedPointer<AbstractCPUMinerWorker> CPUMinerWorkerPtr;

enum class MinerThreadState: quint8 {Mining, Exit, UpdateJob, Stopping};
enum class GpuMinerError: quint8 { NoError, Unknown, NoMemory, OclVersionNotSupported };
enum class CudaDeviceParam: quint8 {GridSize, BlockSize, Intensity};
enum class OclDeviceParam: quint8 {GlobalWorkSize, LocalWorkSize};
enum class ImplType: quint8 {CUDA, OpenCL, FirstType = ImplType::CUDA, LastType = ImplType::OpenCL};

inline const ImplType operator ++(ImplType& a) {
  a = static_cast<ImplType>(static_cast<int>(a) + 1);
  return a;
}

#ifndef NO_ETH
typedef quint64 Nonce;
#else
typedef quint32 Nonce;
#endif

}
