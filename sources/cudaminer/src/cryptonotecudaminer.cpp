#include "cudaminer/cryptonotecudaminer.h"
#include "cudaminer/cudaminer_p.h"
#include "cudaminer/cudadeviceprops.h"

#include "miner-abstract/minerabstractcommonprerequisites.h"
#include "miner-abstract/abstractgpumanager.h"
#include "miner-abstract/noncectl.h"

#include "hash.h"
#include "cuda.h"
#include "cuda_runtime.h"
#include "cu_cryptonote/cudadefs.h"

#include "utils/settings.h"

extern bool cryptonight_core_cpu_init(int deviceNo);
extern void cryptonight_core_cpu_hash(int deviceNo, int blocks, int threads, quint32 *d_long_state, struct cryptonight_gpu_ctx *d_ctx, int device_bfactor, int cuda_major);
extern bool cryptonight_extra_cpu_init(int deviceNo);
extern void cryptonight_extra_cpu_prepare(int deviceNo, int threads, quint32 *d_input, quint32 startNonce, struct cryptonight_gpu_ctx *d_ctx);
extern void cryptonight_extra_cpu_final(int deviceNo, int threads, quint32 *d_target, quint32 *d_resultNonce, quint32 startNonce, quint32 *nonce, struct cryptonight_gpu_ctx *d_ctx);

namespace MinerGate {

const int MIN_TEST_MS = 100;
const quint16 DEFAULT_GRID_SIZE = 8;

class CryptonoteCudaMinerPrivate: public CudaMinerPrivate {
public:
  quint32 *m_long_state = nullptr;
  quint32 m_throughput = 0;
  quint32 m_blocks = 0;
  quint32 m_threads = 0;

  quint32 *m_cuda_input = nullptr;
  quint32 *m_cuda_target = nullptr;
  quint32 *m_cuda_nonce = nullptr;

  cryptonight_gpu_ctx *m_ctx = nullptr;

  CryptonoteCudaMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
    CudaMinerPrivate(_device, _manager) {
  }

  ~CryptonoteCudaMinerPrivate() {
    if(m_long_state)
      cudaFree(m_long_state);
    if(m_ctx)
      cudaFree(m_ctx);
    if(m_cuda_input)
      cudaFree(m_cuda_input);
    if(m_cuda_target)
      cudaFree(m_cuda_target);
    if(m_cuda_nonce)
      cudaFree(m_cuda_nonce);
  }

  bool init() {
    m_manager->info(QString("Initializing Cryptonote CUDA miner for device %1...").arg(m_device));
    size_t free(0), total(0);
    if (cudaSetDevice(m_device) != cudaSuccess) {
      m_manager->critical(QString("cudaSetDevice error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }
    if (cudaDeviceReset() != cudaSuccess) {
      m_manager->critical(QString("cudaDeviceReset error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }
    if (cudaSetDeviceFlags(cudaDeviceScheduleBlockingSync) != cudaSuccess) {
      m_manager->critical(QString("cudaSetDeviceFlags error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }
    if (cudaMemGetInfo(&free, &total) != cudaSuccess) {
      m_manager->critical(QString("cudaMemGetInfo error: %1").arg(cudaGetErrorString(cudaGetLastError())));
      return false;
    }

    const CudaDevicePropPtr &devProps = m_manager->deviceProps(m_device).value<CudaDevicePropPtr>();

    m_manager->info(QString("Device name: %1").arg(devProps->name));
    m_manager->info(QString("Total memory: %1").arg(total));
    m_manager->info(QString("Free memory: %1").arg(free));
    m_manager->info(QString("MP count: %1").arg(devProps->multiProcessorCount));
    m_manager->info(QString("MP threads count: %1").arg(devProps->maxThreadsPerMultiProcessor));
    m_manager->info(QString("CUDA version: %1.%2").arg(devProps->major).arg(devProps->minor));
    m_manager->info(QString("CUDA cores: %1").arg(devProps->multiProcessorCount * cudaCoresPerMP(devProps->major, devProps->minor)));
    m_manager->info(QString("Threads per block: %1").arg(devProps->maxThreadsPerBlock));
    m_manager->info(QString("Dim size: %1 | %2 | %3").arg(devProps->maxThreadsDim[0])
        .arg(devProps->maxThreadsDim[1])
        .arg(devProps->maxThreadsDim[2]));
    m_manager->info(QString("Grid size: %1 | %2 | %3").arg(devProps->maxGridSize[0])
        .arg(devProps->maxGridSize[1])
        .arg(devProps->maxGridSize[2]));

    {
      QMutexLocker l(&m_paramsMutex);
      if (m_deviceParams.contains((int)CudaDeviceParam::GridSize))
        m_threads = m_deviceParams.value((int)CudaDeviceParam::GridSize).value<quint32>();
      else
        m_threads = DEFAULT_GRID_SIZE;

      if (m_deviceParams.contains((int)CudaDeviceParam::BlockSize))
        m_blocks = m_deviceParams.value((int)CudaDeviceParam::BlockSize).value<quint32>();
      else
        m_blocks = calcBlockSize(m_threads);
    }

    m_throughput = m_threads * m_blocks;

    m_manager->info(QString("Calculated threads per block: %1").arg(m_threads));
    m_manager->info(QString("Calculated blocks: %1").arg(m_blocks));
    m_manager->info(QString("Total threads: %1").arg(m_throughput));

    if (cudaMalloc(&m_ctx, sizeof(struct cryptonight_gpu_ctx) * m_throughput) != cudaSuccess) {
      m_manager->critical(QString("cudaMalloc context: %1").arg(cudaGetErrorString(cudaGetLastError())));
      m_ctx = nullptr;
      m_lastError = GpuMinerError::NoMemory;
      return false;
    }

    if(cudaMalloc(&m_cuda_input, 19 * sizeof(quint32)) != cudaSuccess ||
       cudaMalloc(&m_cuda_target, 8 * sizeof(quint32)) != cudaSuccess ||
       cudaMalloc(&m_cuda_nonce, sizeof(quint32)) != cudaSuccess) {
      m_manager->critical(QString("cudaMalloc variables: %1").arg(cudaGetErrorString(cudaGetLastError())));
      cudaFree(m_ctx);

      if(m_cuda_input) cudaFree(m_cuda_input);
      if(m_cuda_target) cudaFree(m_cuda_target);
      if(m_cuda_nonce) cudaFree(m_cuda_nonce);

      m_ctx = nullptr;
      m_cuda_input = nullptr;
      m_cuda_target = nullptr;
      m_cuda_nonce = nullptr;
      return false;
    }

    if(!cryptonight_core_cpu_init(m_device) || !cryptonight_extra_cpu_init(m_device)) {
      m_manager->critical(cudaGetErrorString(cudaGetLastError()));
      cudaFree(m_ctx);
      cudaFree(m_cuda_input);
      cudaFree(m_cuda_target);
      cudaFree(m_cuda_nonce);
      m_ctx = nullptr;
      m_cuda_input = nullptr;
      m_cuda_target = nullptr;
      m_cuda_nonce = nullptr;
      return false;
    }

    while(cudaMalloc<quint32>(&m_long_state, MEMORY * m_throughput) != cudaSuccess) {
      --m_blocks;
      m_throughput = m_threads * m_blocks;

      if(!m_blocks) {
        m_manager->critical(QString("cudaMalloc long state: %1").arg(cudaGetErrorString(cudaGetLastError())));

        cudaFree(m_ctx);
        cudaFree(m_cuda_input);
        cudaFree(m_cuda_target);
        cudaFree(m_cuda_nonce);

        m_long_state = nullptr;
        m_ctx = nullptr;
        m_cuda_input = nullptr;
        m_cuda_target = nullptr;
        m_cuda_nonce = nullptr;

        m_lastError = GpuMinerError::NoMemory;
        return false;
      }
    }

    m_lastError = GpuMinerError::NoError;
    m_manager->info("CUDA miner successfully initialized");
    return true;
  }

  bool reinit() {
    cudaSetDevice(m_device);
    cudaFree(m_long_state);
    cudaFree(m_ctx);
    cudaFree(m_cuda_input);
    cudaFree(m_cuda_target);
    cudaFree(m_cuda_nonce);
    m_long_state = nullptr;
    m_ctx = nullptr;
    m_cuda_input = nullptr;
    m_cuda_target = nullptr;
    m_cuda_nonce = nullptr;
    return init();
  }
};

CryptonoteCudaMiner::CryptonoteCudaMiner(quint32 _device, AbstractGPUManager *_manager):
  CudaMiner(*new CryptonoteCudaMinerPrivate(_device, _manager)) {
}

CryptonoteCudaMiner::~CryptonoteCudaMiner() {
}

void CryptonoteCudaMiner::start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex,
                                QMutex &_feeJobMutex, volatile std::atomic<MinerThreadState> &_state, bool _fee) {
  Q_D(CryptonoteCudaMiner);

  {
    int deviceNo;
    d->reinit();
    cudaGetDevice(&deviceNo);
    Q_ASSERT(deviceNo == d->m_device);
  }
  d->m_hashCount = 0;
  const CudaDevicePropPtr &dev_props = d->m_manager->deviceProps(d->m_device).value<CudaDevicePropPtr>();
  quint32 target[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0};
  quint32 finishNonce = 0, startNonce = 0;
  quint32 finishFeeNonce = 0, startFeeNonce = 0;
  quint32 *nonceptr;
  quint32 *feeNoncePtr;
  QString feeJobId;

  crypto::hash hash;
  crypto::cn_context context;

  MiningJob job;
  NonceCtl *nonceCtl = NonceCtl::instance(d->m_coinKey, false);
  NonceCtl *nonceFeeCtl = NonceCtl::instance(d->m_coinKey, true);

  Q_FOREVER {
    switch(_state) {
    case MinerThreadState::Exit:
    case MinerThreadState::Stopping:
      d->m_hashCount = 0;
      cudaDeviceReset();
      return;

    case MinerThreadState::UpdateJob: {
      {
        QMutexLocker lock(&_jobMutex);
        job = _job;
      }
      if (!job.job_id.isEmpty()) {
        target[7] = arrayToQuint32(job.target);

        nonceptr = (quint32*)(((char*)job.blob.data()) + 39);
        *nonceptr = 0;
        Q_FOREVER {
          if (!nonceCtl->isValid())
            QThread::msleep(100);
          else {
            finishNonce = nonceCtl->nextNonce(d->m_throughput);
            break;
          }
          if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
            break;
        }
        if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
          continue;
        startNonce = finishNonce - d->m_throughput + 1;

        if(cudaMemcpy(d->m_cuda_input, job.blob.data(), 19 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess ||
           cudaMemcpy(d->m_cuda_target, target, 8 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess) {
          cudaError err = cudaGetLastError();
          if(err != cudaSuccess) {
            d->m_manager->critical("1:" + QString(cudaGetErrorString(err)));
            d->m_hashCount = 0;
            cudaDeviceReset();
            return;
          }
          continue;
        }
        if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
          continue;
        _state = MinerThreadState::Mining;
      }
      else {
        QThread::sleep(3);
        continue;
      }
      break;
    }

    case MinerThreadState::Mining: {
      quint32 intensity;
      {
        QMutexLocker l(&d->m_paramsMutex);
        intensity = d->m_deviceParams.value((int)CudaDeviceParam::Intensity).value<quint32>();
      }
      quint32 deviceLoading = intensity2deviceFactor(intensity);
      if(Q_UNLIKELY(_fee && qrand() % 100 < 2)) {
        //fee
        QMutexLocker lock(&_feeJobMutex);
        if(Q_UNLIKELY(_feeJob.job_id.isEmpty()))
          continue;
        target[7] = arrayToQuint32(_feeJob.target);
        if(feeJobId != _feeJob.job_id)
          feeJobId = _feeJob.job_id;
        while (_state == MinerThreadState::Mining)
          if (!nonceFeeCtl->isValid())
            QThread::msleep(100);
          else {
            finishFeeNonce = nonceFeeCtl->nextNonce(d->m_throughput);
            break;
          }
        if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
          continue;
        startFeeNonce = finishFeeNonce - d->m_throughput + 1;
        feeNoncePtr = (quint32*)(((char*)_feeJob.blob.data()) + 39);

        if(cudaMemcpy(d->m_cuda_input, _feeJob.blob.data(), 19 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess ||
           cudaMemcpy(d->m_cuda_target, target, 8 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess) {
          cudaError err = cudaGetLastError();
          if(err != cudaSuccess) {
            d->m_manager->critical("2:" + QString(cudaGetErrorString(err)));
            d->reinit();
          }
          continue;
        }

        quint32 foundNonce = 0xFFFFFFFF;
        cryptonight_extra_cpu_prepare(d->m_device, d->m_throughput, d->m_cuda_input, startFeeNonce, d->m_ctx);
        cryptonight_core_cpu_hash(d->m_device, d->m_blocks, d->m_threads, d->m_long_state, d->m_ctx, deviceLoading,
                                  dev_props->major);
        cryptonight_extra_cpu_final(d->m_device, d->m_throughput, d->m_cuda_target, d->m_cuda_nonce, startFeeNonce, &foundNonce, d->m_ctx);

        if (foundNonce < 0xffffffff) {
          QByteArray tempdata(_feeJob.blob);
          std::memcpy(&tempdata.data()[39], &foundNonce, sizeof(foundNonce));
          std::memset(&hash, 0, sizeof(hash));

          crypto::cn_slow_hash(context, tempdata.data(), tempdata.size(), hash);

          if(((quint32*)&hash)[7] <=  arrayToQuint32(_feeJob.target)) {
            const QByteArray hashArray(reinterpret_cast<const char*>(&hash), sizeof(hash));
            Q_EMIT shareFound(_feeJob.job_id, hashArray, foundNonce, true);
          }
          else
            fprintf(stderr, "GPU #%d: result for fee nonce $%08X does not validate on CPU!\n", d->m_device, foundNonce);
        }

        target[7] = arrayToQuint32(job.target);
        if(cudaMemcpy(d->m_cuda_input, job.blob.data(), 19 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess ||
           cudaMemcpy(d->m_cuda_target, target, 8 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess) {
          cudaError err = cudaGetLastError();
          if (err != cudaSuccess) {
            d->m_manager->critical("3:" + QString(cudaGetErrorString(err)));
            d->reinit();
            continue;
          }
        }
      }
      else {
        quint32 foundNonce = 0xFFFFFFFF;

        cryptonight_extra_cpu_prepare(d->m_device, d->m_throughput, d->m_cuda_input, startNonce, d->m_ctx);
        cryptonight_core_cpu_hash(d->m_device, d->m_blocks, d->m_threads, d->m_long_state, d->m_ctx, deviceLoading, dev_props->major);
        cryptonight_extra_cpu_final(d->m_device, d->m_throughput, d->m_cuda_target, d->m_cuda_nonce, startNonce, &foundNonce, d->m_ctx);
        if (Q_UNLIKELY(foundNonce < 0xffffffff)) {
          d->m_manager->info(QString("CUDA share found, nonce = %1").arg(foundNonce));
          d->m_hashCount +=  foundNonce - startNonce + 1;
          QByteArray tempdata(job.blob);
          std::memcpy(&tempdata.data()[39], &foundNonce, sizeof(foundNonce));
          std::memset(&hash, 0, sizeof(hash));

          crypto::cn_slow_hash(context, tempdata.data(), tempdata.size(), hash);

          if(Q_LIKELY(((quint32*)&hash)[7] <= arrayToQuint32(job.target))) {
            *nonceptr = foundNonce;
            const QByteArray hashArray(reinterpret_cast<const char*>(&hash), sizeof(hash));
            Q_EMIT shareFound(job.job_id, hashArray, foundNonce, false);
          }
          else
            fprintf(stderr, "GPU #%d: result for nonce $%08X does not validate on CPU!\n", d->m_device, foundNonce);
        }
        else
          d->m_hashCount += d->m_throughput;

        while (_state == MinerThreadState::Mining)
          if (!nonceCtl->isValid())
            QThread::msleep(100);
          else {
            finishNonce = nonceCtl->nextNonce(d->m_throughput);
            break;
          }
        if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
          continue;
        startNonce = finishNonce - d->m_throughput + 1;
      }
      break;
    }
    };
  }
  d->m_deviceHRCounter.reset();
}

bool CryptonoteCudaMiner::benchmark(qint64 &_elapsed, quint32 &_hashCount) {
  Q_D(CryptonoteCudaMiner);
  d->init();
  const uint8_t BLOB_SIZE = 152;
  _elapsed = 0;
  quint32 foundNonce = 0xFFFFFFFF;
  quint32 target[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0};
  std::vector<char> blob;
  blob.resize(BLOB_SIZE);
  const CudaDevicePropPtr &devProps = d->m_manager->deviceProps(d->m_device).value<CudaDevicePropPtr>();

  {
    QMutexLocker l(&d->m_paramsMutex);
    if (!d->m_deviceParams.contains((int)CudaDeviceParam::GridSize)) {
      d->m_threads = DEFAULT_GRID_SIZE;
      d->m_deviceParams.insert((int)CudaDeviceParam::GridSize, d->m_threads);
    } else
      d->m_threads = d->m_deviceParams.value((int)CudaDeviceParam::GridSize).value<quint32>();

    if (!d->m_deviceParams.contains((int)CudaDeviceParam::BlockSize)) {
      d->m_blocks = d->calcBlockSize(d->m_threads);
      d->m_deviceParams.insert((int)CudaDeviceParam::BlockSize, d->m_blocks);
    } else
      d->m_blocks = d->m_deviceParams.value((int)CudaDeviceParam::BlockSize).value<quint32>();
    d->m_throughput = d->m_blocks * d->m_threads;
  }

  _hashCount = 0;
  const quint8 TEST_COUNT(3);
  QElapsedTimer timer;
  timer.start();
  for (quint8 i = 0; i < TEST_COUNT; i++) {
    //    target[7] = 0; // always 0!
    std::fill(blob.begin(), blob.end(), i + 1);

    if(cudaMemcpy(d->m_cuda_input, blob.data(), 19 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess ||
       cudaMemcpy(d->m_cuda_target, target, 8 * sizeof(quint32), cudaMemcpyHostToDevice) != cudaSuccess) {
      cudaError err = cudaGetLastError();
      if (err != cudaSuccess)
        d->m_manager->critical("BM-4:" + QString(cudaGetErrorString(err)));
      _hashCount = 0;
      return false;
    }

    const int B_FACTOR = 6;
    cryptonight_extra_cpu_prepare(d->m_device, d->m_throughput, d->m_cuda_input, 1, d->m_ctx);
    cryptonight_core_cpu_hash(d->m_device, d->m_blocks, d->m_threads, d->m_long_state, d->m_ctx, B_FACTOR, devProps->major);
    cryptonight_extra_cpu_final(d->m_device, d->m_throughput, d->m_cuda_target, d->m_cuda_nonce, 1, &foundNonce, d->m_ctx);
    _hashCount += d->m_throughput;
  }
  _elapsed = timer.elapsed();
  if (_elapsed < MIN_TEST_MS)
    return false;
  else
    return true;
}

}
