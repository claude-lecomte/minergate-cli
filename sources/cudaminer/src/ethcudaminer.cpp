#include "cudaminer/ethcudaminer.h"
#include "cudaminer/cudaminer_p.h"
#include "cudaminer/cudadeviceprops.h"

#include "cu_ethereum/ethash_cuda_miner_kernel.h"

#include "miner-abstract/abstractgpumanager.h"
#include "miner-abstract/noncectl.h"

#include "utils/settings.h"
#include "utils/logger.h"

#include "libethcore/EthashAux.h"

#include "libethash/internal.h"
#include "libethash/data_sizes.h"

#include "ethutils/dagcontroller.h"

#include <random>
#include <QtMath>

#include "cuda.h"
#include "cuda_runtime.h"

namespace MinerGate {

const quint32 EXTRA_GPU_MEMORY(314572800 / 2); // keep 150 Mb for OS
const quint8 MAX_GPU_STREAMS(5); // round of calculations before getting nonce's from GPU
const quint64 NONCE_INTERVAL(1000000000);
const quint64 MINIMAL_GPU_MEMORY(2000000000); // <~ 2 Gb
const quint8 STARTING_EPOCH = 50;
const QList<quint16> AVAILABLE_BLOCK_SIZE = {32u, 64u, 128u, 256u};
const quint8 DEFAULT_BLOCK_SIZE_INDEX = 1;

class EthCUDAMinerPrivate: public CudaMinerPrivate {
public:
  QScopedPointer<volatile quint32*> m_searchBuf;
  QScopedPointer<cudaStream_t> m_streams;
  hash32_t m_currentHeader;
  quint64 m_currentTarget;
  quint64 m_currentIndex;

  quint16 m_blockSize = 0;
  quint32 m_gridSize = 0;

  EthCUDAMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
    CudaMinerPrivate(_device, _manager) {
  }

  bool init() {
    m_manager->info("Initializing Ethereum CUDA miner...");
    if (cudaSetDevice(m_device) != cudaSuccess) {
      m_manager->critical(QString("cudaSetDevice error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }

    if (resetDevice() != cudaSuccess) {
      m_manager->critical(QString("d->resetDevice error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }
    cudaDeviceSetCacheConfig(cudaFuncCachePreferL1); //TODO: try set for cryptonotes
    return true;
  }

  cudaError resetDevice() {
    return cudaDeviceReset();
  }

};

EthCUDAMiner::EthCUDAMiner(quint32 _device, AbstractGPUManager *_manager):
  CudaMiner(*new EthCUDAMinerPrivate(_device, _manager)) {
}


void EthCUDAMiner::start(MiningJob &_job, MiningJob &, QMutex &_jobMutex, QMutex &,
                         volatile std::atomic<MinerThreadState> &_state, bool) {
  Q_D(EthCUDAMiner);
  if (!d->init())
    return;

  using namespace dev;
  using namespace dev::eth;

  MiningJob job;
  bool capabilityVerified = false;
  QByteArray lastSeedHash;
  h256 seedHash;
  cudaError err = cudaSuccess;
  NonceCtl *nonceCtl = NonceCtl::instance(d->m_coinKey, false);
  hash128_t *gpuDag = nullptr;

  {
    QMutexLocker l(&d->m_paramsMutex);
    if (!d->m_deviceParams.contains((int)CudaDeviceParam::GridSize)) {
      d->m_gridSize = Settings::instance().ethCudaGridSize(d->m_device);
      d->m_deviceParams.insert((int)CudaDeviceParam::GridSize, d->m_blockSize);
    } else
      d->m_gridSize = d->m_deviceParams.value((int)CudaDeviceParam::GridSize).value<quint32>();

    if (!d->m_deviceParams.contains((int)CudaDeviceParam::BlockSize)) {
      d->m_blockSize = AVAILABLE_BLOCK_SIZE.at(DEFAULT_BLOCK_SIZE_INDEX);
      d->m_deviceParams.insert((int)CudaDeviceParam::BlockSize, d->m_gridSize);
    } else
      d->m_blockSize = d->m_deviceParams.value((int)CudaDeviceParam::BlockSize).value<quint32>();
  }

  Q_FOREVER {
    switch (_state) {
    case MinerThreadState::Exit:
    case MinerThreadState::Stopping:
      d->m_hashCount = 0;
      if (gpuDag) {
        cudaFree(gpuDag);
        gpuDag = nullptr;
      }
      d->resetDevice();
      return;

    case MinerThreadState::UpdateJob: {
      {
        QMutexLocker lock(&_jobMutex);
        job = _job;
      }
      if (job.blob.isEmpty()) {
        Logger::info(QString::null, "ETH CUDA: waiting for job");
        QThread::sleep(3);
        continue;
      }

      if (!capabilityVerified || lastSeedHash != job.blob) {
        capabilityVerified = true;
        bool ok = false;

        try {
          ok = checkCardCapability(job);
        } catch (...) {
          _state = MinerThreadState::Stopping;
          d->resetDevice();
          return;
        }

        if (!ok) {
          _state = MinerThreadState::Stopping;
          d->resetDevice();
          return;
        }
      }

      if (lastSeedHash != job.blob) {
        const DagController &dagCtl = DagController::instance();
        const DagType &dag = dagCtl.dag();
        if (!dag) {
          Logger::info("eth", "DAG not ready");
          QThread::sleep(3);
          continue;
        }

        d->m_searchBuf.reset(new volatile quint32 *[MAX_GPU_STREAMS]);
        d->m_streams.reset(new cudaStream_t[MAX_GPU_STREAMS]);

        // copy DAG to GPU memory
        seedHash = arrayToH256(job.blob);
        quint64 blockNumber = EthashAux::number(seedHash);
        // check available GPU memory size
        quint64 dagSize = ethash_get_datasize(blockNumber);
        quint32 dagSize128 = (unsigned)(dagSize / ETHASH_MIX_BYTES);
        // create buffer for dag
        if (gpuDag) {
          cudaFree(gpuDag);
          gpuDag = nullptr;
        }

        Logger::debug(d->m_coinKey, QString("Trying to allocate %1 bytes of GPU memory...").arg(dagSize));
        err = cudaMalloc(reinterpret_cast<void**>(&gpuDag), dagSize);
        if (err != cudaSuccess) {
          Logger::critical(QString::null, QString("can't allocate memory for DAG: %1").arg(cudaGetErrorString(err)));
          d->m_lastError = GpuMinerError::NoMemory;
          d->resetDevice();
          return;
        }

        // copy dag to GPU.
        auto dagSrc = dag.get()->data().data();
        if (cudaSuccess != cudaMemcpy(reinterpret_cast<void*>(gpuDag), dagSrc, dagSize, cudaMemcpyHostToDevice)) {
          Logger::critical(QString::null, "can't copy DAG to GPU device");
          d->resetDevice();
          return;
        }

        // create mining buffers
        for (quint8 i = 0; i < MAX_GPU_STREAMS; i++) {
          if (cudaSuccess != cudaMallocHost(&d->m_searchBuf.data()[i], SEARCH_RESULT_BUFFER_SIZE * sizeof(quint32))) {
            Logger::critical(QString::null, "can't create buffer DAG to GPU device");
            d->resetDevice();
            return;
          }

          err = cudaStreamCreate(&d->m_streams.data()[i]);
          if (cudaSuccess != err) {
            Logger::critical(QString::null, tr("can't create CUDA stream #%1 with error: %2").arg(i).arg(cudaGetErrorString(err)));
            d->resetDevice();
            return;
          }
        }

        set_constants(gpuDag, dagSize128);
        lastSeedHash = job.blob;
      }

      memset(&d->m_currentHeader, 0, sizeof(quint32));
      d->m_currentTarget = 0;
      d->m_currentIndex = 0;
      if (_state == MinerThreadState::UpdateJob)
        _state = MinerThreadState::Mining;
      break;
    }
    case MinerThreadState::Mining:
      bool initialize = false;
      bool exit = false;
      const h256 headerHash = arrayToH256(job.job_id);
      const h256 boundary = arrayToH256(job.target);
      const quint8 *header = headerHash.data();
      const quint64 target = (quint64)(u64)((u256)boundary >> 192);

      if (memcmp(&d->m_currentHeader, header, sizeof(hash32_t))) {
        d->m_currentHeader = *reinterpret_cast<const hash32_t *>(header);
        set_header(d->m_currentHeader);
        initialize = true;
      }

      if (d->m_currentTarget != target) {
        d->m_currentTarget = target;
        set_target(d->m_currentTarget);
        initialize = true;
      }

      const quint32 usedBlockSize = d->m_blockSize;
      const quint64 batchSize = d->m_gridSize * usedBlockSize;
      const quint64 lastNonce = nonceCtl->nextNonce(NONCE_INTERVAL);
      quint64 nonce = lastNonce - NONCE_INTERVAL;

      if (initialize) {
        d->m_currentIndex = 0;
        if (cudaSuccess != cudaDeviceSynchronize()) {
          Logger::critical(QString::null, "ETH CUDA error");
          d->resetDevice();
          return;
        }
        Q_ASSERT(d->m_searchBuf);
        for (quint8 i = 0; i < MAX_GPU_STREAMS; i++)
          d->m_searchBuf.data()[i][0] = 0;
      }

      quint64 nonces[SEARCH_RESULT_BUFFER_SIZE - 1];

      for (; !exit && nonce + batchSize <= lastNonce && _state == MinerThreadState::Mining; d->m_currentIndex++, nonce += batchSize) {
        const quint8 streamIndex = d->m_currentIndex % MAX_GPU_STREAMS;
        const cudaStream_t &stream = d->m_streams.data()[streamIndex];
        volatile quint32* buffer = d->m_searchBuf.data()[streamIndex];
        quint32 foundCount = 0;
        const quint64 nonce_base = nonce - MAX_GPU_STREAMS * batchSize;
        if (d->m_currentIndex >= MAX_GPU_STREAMS) {
          if (cudaSuccess != cudaStreamSynchronize(stream)) {
            Logger::critical(QString::null, "ETH CUDA error - 2");
            d->resetDevice();
            return;
          }

          foundCount = buffer[0];
          if (foundCount)
            buffer[0] = 0;
          for (unsigned int j = 0; j < foundCount; j++)
            nonces[j] = nonce_base + buffer[j + 1];
        }

        try {
          run_ethash_search(d->m_device, d->m_gridSize, usedBlockSize, stream, buffer, nonce);
        } catch (...) {
          Logger::critical(QString::null, "Critical ethash error");
          d->resetDevice();
          return;
        }

        if (d->m_currentIndex >= MAX_GPU_STREAMS) {
          exit = foundCount;
          for (quint32 i = 0; i < foundCount; ++i) {
            const quint64 foundNonce = nonces[i];
            const dev::eth::Nonce n = (dev::eth::Nonce)(u64)foundNonce;
            try {
              const EthashProofOfWork::Result &evalResult = EthashAux::eval(seedHash, headerHash, n);
              if (evalResult.value < boundary)
                Q_EMIT shareFound(job.job_id, h256ToArray(evalResult.mixHash), foundNonce, false);
            } catch (...) {
              Logger::critical("ETH", "Error call EthashAux::eval");
              continue;
            }
          }
          d->m_hashCount += batchSize;
        }
      }
      break;
    }
  }
  if (gpuDag) {
    cudaFree(gpuDag);
    gpuDag = nullptr;
  }
  d->resetDevice();
  d->m_deviceHRCounter.reset();
}

bool EthCUDAMiner::benchmark(qint64 &_elapsed, quint32 &_hashCount) {
  Q_D(EthCUDAMiner);
  if (!d->init())
    return false;

  {
    QMutexLocker l(&d->m_paramsMutex);
    if (!d->m_deviceParams.contains((int)CudaDeviceParam::GridSize)) {
      d->m_gridSize = Settings::instance().ethCudaGridSize(d->m_device);
      d->m_deviceParams.insert((int)CudaDeviceParam::GridSize, d->m_gridSize);
    } else
      d->m_gridSize = d->m_deviceParams.value((int)CudaDeviceParam::GridSize).value<quint32>();

    if (!d->m_deviceParams.contains((int)CudaDeviceParam::BlockSize)) {
      d->m_blockSize = AVAILABLE_BLOCK_SIZE.at(DEFAULT_BLOCK_SIZE_INDEX);
      d->m_deviceParams.insert((int)CudaDeviceParam::BlockSize, d->m_blockSize);
    } else
      d->m_blockSize = d->m_deviceParams.value((int)CudaDeviceParam::BlockSize).value<quint32>();
  }

  using namespace dev;
  using namespace dev::eth;
  hash128_t *gpuDag = nullptr;
  bool ok = false;
  try {
    ok = checkCardCapability(d->m_testJob);
  } catch (...) {
    return false;
  }

  if (!ok)
    return false;

  DagController &dagCtl = DagController::instance();
  std::atomic<bool> inProgress(true);
  PreparingError error = PreparingError::NoError;
  if (!dagCtl.prepareTestDag(d->m_testJob.blob, inProgress, error))
    return false;
  const DagType &dag = dagCtl.dag();
  if (!dag) {
    qCritical() << "DAG not ready";
    return false;
  }

  d->m_searchBuf.reset(new volatile quint32 *[1]);
  d->m_streams.reset(new cudaStream_t[1]);

  // copy DAG to GPU memory
  h256 seedHash = dagCtl.currentSeedhash();
  quint64 blockNumber = EthashAux::number(seedHash);
  // check available GPU memory size
  quint64 dagSize = ethash_get_datasize(blockNumber);
  quint32 dagSize128 = (unsigned)(dagSize / ETHASH_MIX_BYTES);
  // create buffer for dag
  Logger::debug(d->m_coinKey, QString("Trying to allocate %1 bytes of GPU memory...").arg(dagSize));
  cudaError err = cudaMalloc(reinterpret_cast<void**>(&gpuDag), dagSize);
  if (err != cudaSuccess) {
    Logger::critical(QString::null, QString("can't allocate memory for DAG: %1").arg(cudaGetErrorString(err)));
    d->m_lastError = GpuMinerError::NoMemory;
    d->resetDevice();
    return false;
  }

  // copy dag to GPU.
  auto dagSrc = dag.get()->data().data();
  if (cudaSuccess != cudaMemcpy(reinterpret_cast<void*>(gpuDag), dagSrc, dagSize, cudaMemcpyHostToDevice)) {
    Logger::critical(QString::null, "can't copy DAG to GPU device");
    d->resetDevice();
    return false;
  }

  {
    // create mining buffers
    if (cudaSuccess != cudaMallocHost(&d->m_searchBuf.data()[0], SEARCH_RESULT_BUFFER_SIZE * sizeof(quint32))) {
      Logger::critical(QString::null, "can't create buffer DAG to GPU device");
      cudaFree(gpuDag);
      d->resetDevice();
      return false;
    }

    err = cudaStreamCreate(&d->m_streams.data()[0]);
    if (cudaSuccess != err) {
      Logger::critical(QString::null, tr("can't create CUDA stream with error: %2").arg(cudaGetErrorString(err)));
      cudaFree(gpuDag);
      d->resetDevice();
      return false;
    }
  }

  set_constants(gpuDag, dagSize128);
  memset(&d->m_currentHeader, 0, sizeof(quint32));

  const h256 headerHash = arrayToH256(d->m_testJob.job_id);
  const h256 boundary = arrayToH256(d->m_testJob.target);
  const quint8 *header = headerHash.data();
  const quint64 target = (quint64)(u64)((u256)boundary >> 192);

  if (memcmp(&d->m_currentHeader, header, sizeof(hash32_t))) {
    d->m_currentHeader = *reinterpret_cast<const hash32_t *>(header);
    set_header(d->m_currentHeader);
  }

  if (d->m_currentTarget != target) {
    d->m_currentTarget = target;
    set_target(d->m_currentTarget);
  }

  const quint64 batchSize = d->m_gridSize * d->m_blockSize;
  quint64 nonce = 0L;

  if (cudaSuccess != cudaDeviceSynchronize()) {
    Logger::critical(QString::null, "ETH CUDA error");
    cudaFree(gpuDag);
    d->resetDevice();
    return false;
  }
  Q_ASSERT(d->m_searchBuf);
  d->m_searchBuf.data()[0][0] = 0;

  const cudaStream_t &stream = d->m_streams.data()[0];
  volatile quint32* buffer = d->m_searchBuf.data()[0];

  const quint8 SKIPPED_COUNT = 2; //skip N first tests
  const quint16 TEST_COUNT = 1000; // count of valid tests
  QElapsedTimer timer;
  qDebug() << "start testing...";
  for (quint16 i = 0; i < TEST_COUNT + SKIPPED_COUNT; i++) {
    if (i == SKIPPED_COUNT)
      timer.start();
    try {
      run_ethash_search(d->m_device, d->m_gridSize, d->m_blockSize, stream, buffer, nonce);
    } catch (...) {
      Logger::critical(QString::null, "Critical ethash error");
      cudaFree(gpuDag);
      d->resetDevice();
      return false;
    }
    if (i >= SKIPPED_COUNT)
      nonce += batchSize;
  }
  _elapsed = timer.elapsed();
  qDebug() << "finishing testing...";
  if (!_elapsed) {
    qCritical() << "zero test time";
    cudaFree(gpuDag);
    d->resetDevice();
    return false;
  }
  _hashCount = nonce;

  cudaFree(gpuDag);
  d->resetDevice();
  return true;
}

bool EthCUDAMiner::allowed() const {
  return checkCardCapability();
}

quint32 EthCUDAMiner::intensity2deviceFactor(quint8 _intensity) const {
  if (_intensity < 1)
    _intensity = 1;
  else if (_intensity > 4)
    _intensity = 4;
  return static_cast<quint32>(qPow(2, 4 - _intensity));
}

quint8 EthCUDAMiner::deviceFactor2Intensity(quint32 _deviceFactor) const {
  return static_cast<quint8>(4 - log2(_deviceFactor));
}

GpuMinerError EthCUDAMiner::checkCardCapability(const AbstractGPUManager *_manager, quint32 _device, const MiningJob &_job) {
  using namespace dev;
  using namespace dev::eth;
  // check available GPU memory size
  quint64 requiredSize;
  quint64 dagSize;
  if (!_job.blob.isEmpty()) {
    h256 seedHash = arrayToH256(_job.blob);
    quint64 blockNumber = EthashAux::number(seedHash);
    dagSize = ethash_get_datasize(blockNumber);
    requiredSize = dagSize + EXTRA_GPU_MEMORY;
  } else {
    requiredSize = MINIMAL_GPU_MEMORY;
    dagSize = dag_sizes[STARTING_EPOCH];
  }
  const CudaDevicePropPtr &deviceProps = _manager->deviceProps(_device);
  if (deviceProps->totalGlobalMem < requiredSize) {
    Logger::critical(QString::null, QString("[CUDA] GPU memory size too small for working ETH miner (required %1 bytes)").arg(requiredSize, 0, 10));
    return GpuMinerError::NoMemory;
  }

  static bool infoDisplayed = false;
  if (!infoDisplayed) {
    Logger::info(QString::null, "Available CUDA memory size: " + QString::number(deviceProps->totalGlobalMem));
    Logger::info(QString::null, "CUDA memory size for calculation: " + QString::number(requiredSize));
    infoDisplayed = true;
  }
  return GpuMinerError::NoError;
}

bool EthCUDAMiner::checkCardCapability(const MiningJob &_job) const {
  Q_D(const EthCUDAMiner);
  d->m_lastError = checkCardCapability(d->m_manager, d->m_device, _job);
  return GpuMinerError::NoError == d->m_lastError;
}

}
