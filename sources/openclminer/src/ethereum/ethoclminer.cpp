#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "ethereum/ethoclminer.h"

#include "openclminer/openclminer_p.h"
#include "openclminer/openclmanager.h"

#include "miner-abstract/noncectl.h"
#include "miner-abstract/miningjob.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/utilscommonprerequisites.h"

#include "libethcore/EthashAux.h"

#include "libethash/ethash.h"
#include "libethash/internal.h"
#include "libethash/data_sizes.h"

#include "ethutils/dagcontroller.h"

#include "hash.h"

#include "clutils.h"
#include "ethash_cl_miner_kernel.h"

#ifdef Q_OS_MAC
#include <OpenCL/cl_platform.h>
#else
#include <CL/cl_platform.h>
#endif

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate {

#define createBuffer(a, b, c, d) { \
  a = clCreateBuffer(algo->context, b, c, NULL, &retval); \
  if(retval != CL_SUCCESS) { \
    Logger::critical(QString::null, QString(d).arg(retval)); \
    return false; \
  } \
}

#define createKernel(a, b, c) { \
  a = clCreateKernel(algo->program, b, &retval); \
  if(retval != CL_SUCCESS) { \
    Logger::critical(QString::null, QString(c).arg(retval)); \
    return false; \
  } \
}

static void addDefinition(std::string& _source, char const* _id, unsigned _value)
{
  char buf[256];
  sprintf(buf, "#define %s %uu\n", _id, _value);
  _source.insert(_source.begin(), buf, buf + strlen(buf));
}

const quint32 EXTRA_GPU_MEMORY(314572800 / 4); // keep 75 Mb for OS
const quint64 MINIMAL_GPU_MEMORY(2000000000); // <~ 2 Gb
const quint64 NONCE_INTERVAL(1000000000);
const quint8 STARTING_EPOCH = 55;
const quint32 MAX_SEARCH_RESULT = 63;
const quint8 MINING_BUFFER_COUNT = 2;
const quint16 HASH_BATCH_SIZE = 1024;

struct pending_batch {
  uint64_t start_nonce;
  unsigned buf;
};

struct EthereumAlgoContext: public AlgoContext {
  cl_mem dagChunk;
  cl_mem header;
#ifdef Q_OS_WIN
  cl_mem searchBuffers[MINING_BUFFER_COUNT];
#else
  cl_mem searchBuffers[MINING_BUFFER_COUNT] = { 0 };
#endif
  cl_kernel searchKernel = nullptr;

#ifdef Q_OS_WIN
  EthereumAlgoContext() {
    memset(searchBuffers, 0, sizeof(cl_mem) * MINING_BUFFER_COUNT);
  }
#endif

  ~EthereumAlgoContext() {
    try {
      if (dagChunk)
        clReleaseMemObject(dagChunk);
    } catch (...) {
      Logger::critical(QString::null, "Error call clReleaseMemObject(dagChunk)");
    }

    if (header) {
      try {
        clReleaseMemObject(header);
      } catch (...) {
        Logger::critical(QString::null, "Error call clReleaseMemObject(header)");
      }
    }

    try {
      for (int i = 0; i < MINING_BUFFER_COUNT; i++) {
        if (searchBuffers[i])
          clReleaseMemObject(searchBuffers[i]);
      }
      if (searchKernel)
        clReleaseKernel(searchKernel);
    } catch (...) {
      Logger::critical(QString::null, "Error call clReleaseKernel");
    }
  }
};

class EthOCLMinerPrivate: public OpenCLMinerPrivate {
public:

  QQueue<pending_batch> m_pending;
  unsigned m_buf = 0;

  EthOCLMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
    OpenCLMinerPrivate(_device, _manager) {
  }

  bool setupAlgo(const AlgoContextPtr &_algo) {
    EthereumAlgoContext *algo = static_cast<EthereumAlgoContext *>(_algo.data());

    const quint64 dagSize = DagController::instance().currentDagSize();
    const OCLDevicePtr device = m_manager->deviceProps(m_device).value<OCLDevicePtr>();
    cl_int retval;

    // create context for this device only
    try {
      algo->context = clCreateContext(NULL, 1, &device->deviceID, NULL, NULL, &retval);
    } catch (...) {
      Logger::critical(QString::null, "Can't call clCreateContext");
      return false;
    }
    if (retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[ETH:0] Can't create device context: %1").arg(retval));
      return false;
    }

    algo->commandQueues = clCreateCommandQueue(algo->context, device->deviceID, 0, &retval);
    if (retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[ETH:1] OpenCL error %1").arg(retval));
      return false;
    }

    {
      // get params for this device from settings
      quint32 groupSize;
      quint32 multiplier;
      {
        QMutexLocker l(&m_paramsMutex);

        if (m_deviceParams.contains((int)OclDeviceParam::LocalWorkSize))
          groupSize = m_deviceParams.value((int)OclDeviceParam::LocalWorkSize).value<quint32>();
        else {
          groupSize = Settings::instance().clLocalWork(m_device, HashFamily::Ethereum);
          m_deviceParams.insert((int)OclDeviceParam::LocalWorkSize, groupSize);
        }

        if (m_deviceParams.contains((int)OclDeviceParam::GlobalWorkSize))
          multiplier = m_deviceParams.value((int)OclDeviceParam::GlobalWorkSize).value<quint32>();
        else {
          multiplier = Settings::instance().clGlobalWork(m_device, HashFamily::Ethereum);
          m_deviceParams.insert((int)OclDeviceParam::GlobalWorkSize, multiplier);
        }
      }

      if (groupSize > device->maximumWorkGroupSize) {
        Logger::warning(QString::null, QString("globalWorkSize was reduced to maximal value for this GPU device. (From %1 to %2)")
                        .arg(groupSize).arg(device->maximumWorkGroupSize));
        groupSize = device->maximumWorkGroupSize;
      }
      algo->workgroupSize = groupSize;
      algo->maxGlobalWorkSize = algo->workgroupSize * multiplier;
      auto newGlobalSize = algo->maxGlobalWorkSize;
      if (newGlobalSize % algo->workgroupSize != 0)
        newGlobalSize = ((newGlobalSize / algo->workgroupSize) + 1) * algo->workgroupSize;
      algo->globalWorkSize = newGlobalSize;

      Logger::info(QString::null, QString("OCL localWorksize is %1, globalWorksize is %2")
                   .arg(algo->workgroupSize)
                   .arg(algo->globalWorkSize));
    }

    {
      // Load program's file
      std::string code(ETHASH_CL_MINER_KERNEL, ETHASH_CL_MINER_KERNEL + ETHASH_CL_MINER_KERNEL_SIZE);
      addDefinition(code, "GROUP_SIZE", algo->workgroupSize);
      addDefinition(code, "DAG_SIZE", (unsigned)(dagSize / ETHASH_MIX_BYTES));
      addDefinition(code, "ACCESSES", ETHASH_ACCESSES);
      addDefinition(code, "MAX_OUTPUTS", MAX_SEARCH_RESULT);
      addDefinition(code, "PLATFORM", device->platformId);
      addDefinition(code, "COMPUTE", 0);
      const char *kernelSource = code.c_str();

      algo->program = clCreateProgramWithSource(algo->context, 1, &kernelSource, NULL, &retval);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[ETH:2] OpenCL error %1").arg(retval));
        return false;
      }

      retval = clBuildProgram(algo->program, 1, &device->deviceID, NULL, NULL, NULL);

      std::size_t len;
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[ETH:3] OpenCL error %1").arg(retval));
        retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);

        if(retval != CL_SUCCESS) {
          Logger::critical(QString::null, QString("[ETH:4] OpenCL error %1").arg(retval));
          return false;
        }

        QScopedPointer<char> buildLog((char *)malloc(sizeof(char) * (len + 2)));
        retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, len, buildLog.data(), NULL);
        if(retval != CL_SUCCESS) {
          Logger::critical(QString::null, QString("[ETH:5-1] OpenCL error %1").arg(retval));
          return false;
        }

        Logger::critical(QString::null, QString("[ETH:5-2] Compile error, build log:\n%1").arg(buildLog.data()));
        return false;
      } else
        Logger::debug(QString::null, "compile ok");

      cl_build_status status;
      do {
        retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
        if(retval != CL_SUCCESS) {
          Logger::critical(QString::null, QString("[ETH:6] OpenCL error %1").arg(retval));
          QThread::sleep(1);
          continue;
        }
      } while(status == CL_BUILD_IN_PROGRESS);

      retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[ETH:7] OpenCL error %1").arg(retval));
        return false;
      }

      QScopedPointer<char> buildLog((char *)malloc(sizeof(char) * (len + 2)));
      retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, len, buildLog.data(), NULL);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[ETH:8] OpenCL error %1").arg(retval));
        return false;
      }

      Logger::debug(QString::null, QString("build Log:\n%1").arg(buildLog.data()));
      Logger::info(QString::null, "[ETH] cl programm building complete");
    }

    // create buffer for dag
    algo->dagChunk = clCreateBuffer(algo->context, CL_MEM_READ_ONLY, dagSize, NULL, &retval);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[ETH] Can't allocate GPU buffer for DAG: %1").arg(retval));
      m_lastError = GpuMinerError::NoMemory;
      return false;
    }
//    createKernel(algo->hashKernel, "ethash_hash", "[ETH] Can't create hash kernel: %1");
    createKernel(algo->searchKernel, "ethash_search", "[ETH] Can't create search kernel: %1");

    const DagType &dag = DagController::instance().dag();
    Q_ASSERT(dag);
    auto dagSrc = dag.get()->data().data();
    retval = clEnqueueWriteBuffer(algo->commandQueues, algo->dagChunk, CL_TRUE, 0, dagSize, dagSrc, 0, NULL, NULL);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:1] Can't write into buffer: %1").arg(retval));
      return false;
    }

    // create buffer for header
    createBuffer(algo->header, CL_MEM_READ_ONLY, 32, "[ETH] Can't create header buffer");

    retval = clSetKernelArg(algo->searchKernel, 1, sizeof(cl_mem), &algo->header);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:1] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    cl_mem dagChunk = algo->dagChunk;
    retval = clSetKernelArg(algo->searchKernel, 2, sizeof(cl_mem), &dagChunk);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:2] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    unsigned tmp = ~0u;
    retval = clSetKernelArg(algo->searchKernel, 5, sizeof(tmp), &tmp);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:4] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    // create mining buffers
    for (quint8 i = 0; i != MINING_BUFFER_COUNT; ++i)
      createBuffer(algo->searchBuffers[i], CL_MEM_WRITE_ONLY, (MAX_SEARCH_RESULT + 1) * sizeof(quint32), "[ETH] Can't create hash buffer: %1");

    return true;
  }

  bool setKernelArgs(const AlgoContextPtr &_algo, const MiningJob &_job) {
    EthereumAlgoContext *algo = static_cast<EthereumAlgoContext *>(_algo.data());
    const dev::h256 headerHash = dev::arrayToH256(_job.job_id);
    const dev::h256 boundary = dev::arrayToH256(_job.target);
    const quint8 *header = headerHash.data();
    const quint64 target = (quint64)(dev::u64)((dev::u256)boundary >> 192);

    // update header constant buffer
    cl_int retval = clEnqueueWriteBuffer(algo->commandQueues, algo->header, false, 0, 32, header, 0, NULL, NULL);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:1] Can't write buffer: %1").arg(retval));
      return false;
    }

    const quint32 c_zero = 0;
    for (quint8 i = 0; i != MINING_BUFFER_COUNT; ++i) {
      retval = clEnqueueWriteBuffer(algo->commandQueues, algo->searchBuffers[i], false, 0, 4, &c_zero, 0, NULL, NULL);
      if (CL_SUCCESS != retval) {
        Logger::critical(QString::null, QString("[ETH:1] Can't write buffer: %1").arg(retval));
        return false;
      }
    }

    retval = clFinish(algo->commandQueues);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH] Can't call clFinish: %1").arg(retval));
      return false;
    }

    // pass these to stop the compiler unrolling the loops
    retval = clSetKernelArg(algo->searchKernel, 4, sizeof(target), &target);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:3] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    return true;
  }

  bool findShares(const AlgoContextPtr &_algo, QList<Nonce> &results, volatile std::atomic<MinerThreadState> &_state) {
    const OCLDevicePtr device = m_manager->deviceProps(m_device).value<OCLDevicePtr>();
    EthereumAlgoContext *algo = static_cast<EthereumAlgoContext *>(_algo.data());
    if (!algo)
      return false;

    std::size_t globalThreads = algo->globalWorkSize, localThreads = algo->workgroupSize;

    Nonce finishNonce;
    Nonce startNonce;

    NonceCtl *nonceCtl = NonceCtl::instance(m_coinKey, false);
    while (_state == MinerThreadState::Mining)
      if (!nonceCtl->isValid())
        QThread::msleep(100);
      else {
        finishNonce = nonceCtl->nextNonce(globalThreads);
        break;
      }
    startNonce = finishNonce - globalThreads + 1;

    if (_state != MinerThreadState::Mining)
      return true;
    // supply output buffer to kernel
    cl_mem mem = algo->searchBuffers[m_buf];
    cl_int retval = clSetKernelArg(algo->searchKernel, 0, sizeof(cl_mem), &mem);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:fs:1] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    retval = clSetKernelArg(algo->searchKernel, 3, sizeof(Nonce), &startNonce);
    if (CL_SUCCESS != retval) {
      Logger::critical(QString::null, QString("[ETH:fs:1] Can't set kernel argument: %1").arg(retval));
      return false;
    }

    // execute it!
    {
      retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->searchKernel, 1, NULL, &globalThreads, &localThreads, 0, NULL, NULL);
      if (CL_SUCCESS != retval) {
        Logger::critical(QString::null, QString("[ETH-1] Can't calculate hashes: %1").arg(retval));
        return false;
      }
    }

    m_pending.enqueue({startNonce, m_buf});
    m_buf = (m_buf + 1) % MINING_BUFFER_COUNT;

    if (m_pending.size() == MINING_BUFFER_COUNT) {
      // send result when last buf processed
      const pending_batch &batch = m_pending.dequeue();
      // could use pinned host pointer instead
      cl_int retval;
      void *result = clEnqueueMapBuffer(
            algo->commandQueues,
            algo->searchBuffers[batch.buf],
            true,
            CL_MAP_READ,
            0,
            (1 + MAX_SEARCH_RESULT) * sizeof(quint32),
            0, NULL, NULL, &retval
            );
      if (CL_SUCCESS != retval) {
        Logger::critical(QString::null, QString("[ETH] Can't get results: %1").arg(retval));
        return false;
      }
      quint32* resultsBuf = static_cast<quint32*>(result);
      quint32 num_found = qMin(resultsBuf[0], MAX_SEARCH_RESULT);
      for (quint32 i = 0; i != num_found; ++i)
        results << batch.start_nonce + resultsBuf[i + 1];

      retval = clEnqueueUnmapMemObject(
            algo->commandQueues,
            algo->searchBuffers[batch.buf],
            resultsBuf, 0, NULL, NULL);
      if (CL_SUCCESS != retval) {
        Logger::critical(QString::null, QString("[ETH] Can't unmap buffer: %1").arg(retval));
        return false;
      }

      // reset search buffer if we're still going
      if (num_found) {
        const quint32 c_zero = 0;

        retval = clEnqueueWriteBuffer(algo->commandQueues, algo->searchBuffers[batch.buf], true, 0, 4, &c_zero, 0, NULL, NULL);
        if (CL_SUCCESS != retval) {
          Logger::critical(QString::null, QString("[ETH] Can't clear buffer: %1").arg(retval));
          return false;
        }
      }
    }
    m_hashCount += algo->globalWorkSize;
    return true;
  }
};

EthOCLMiner::EthOCLMiner(quint32 _device, AbstractGPUManager *_manager):
  OpenCLMiner(*new EthOCLMinerPrivate(_device, _manager)) {
}

EthOCLMiner::~EthOCLMiner() {
}

void EthOCLMiner::start(MiningJob &_job, MiningJob &, QMutex &_jobMutex, QMutex &,
                        volatile std::atomic<MinerThreadState> &_state, bool) {
  Q_D(EthOCLMiner);
  if (!d->init())
    return;
  d->m_hashCount = 0;
  MiningJob job;
  AlgoContextPtr algo; // scoped algo context
  const DagController &dagCtl = DagController::instance();

  auto setupAlgo = [&algo, d]() -> bool {
    algo.reset(new EthereumAlgoContext);
    return d->setupAlgo(algo);
  };

  Q_FOREVER {
    switch(_state) {
    case MinerThreadState::Exit:
    case MinerThreadState::Stopping:
      d->m_hashCount = 0;
      return;
    case MinerThreadState::UpdateJob: {
      bool empty;
      bool dagChanged = false;
      {
        QMutexLocker lock(&_jobMutex);
        empty = _job.job_id.isEmpty();
        if (!empty) {
          dagChanged = job.blob != dagCtl.currentSeedhashData();
          job = _job;
        }
      }

      if (dagChanged) {
        Logger::debug("d", "resetup device");
        if (!setupAlgo()) {
          Logger::critical(QString::null, "ocl init error:" + QString::number((int)d->m_lastError));
          d->m_hashCount = 0;
          _state = MinerThreadState::Exit;
          return;
        }
      }

      if (empty) {
        Logger::debug("d", "Waiting for non-empty job...");
        QThread::sleep(3);
        continue;
      } else {
        if (!d->setKernelArgs(algo, job)) {
          d->m_manager->critical("Invalid OpenCL params");
          _state = MinerThreadState::Exit;
          return;
        }
        d->m_buf = 0;
        d->m_pending.clear();
        if (_state == MinerThreadState::UpdateJob)
          _state = MinerThreadState::Mining;
      }
      break;
    }
    case MinerThreadState::Mining: {
      QList<Nonce> results;
      if (!d->findShares(algo, results, _state)) {
        d->m_manager->critical("Error call OpenCL program");
        _state = MinerThreadState::Exit;
        continue;
      }
      if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
        continue;
      if (!results.isEmpty()) {
        const dev::h256 seedHash = dev::arrayToH256(job.blob);
        const dev::h256 headerHash = dev::arrayToH256(job.job_id);
        const dev::h256 boundary = dev::arrayToH256(job.target);

        for (Nonce foundNonce: results) {
          // check on CPU and get share
          dev::eth::Nonce n = (dev::eth::Nonce)(dev::u64)foundNonce;
          try {
            const dev::eth::EthashProofOfWork::Result &evalResult = dev::eth::EthashAux::eval(seedHash, headerHash, n);
            if (evalResult.value < boundary)
              Q_EMIT shareFound(job.job_id, dev::h256ToArray(evalResult.mixHash), foundNonce, false);
            else
              Logger::warning(d->m_coinKey, "Low difficulty share!");
          } catch (...) {
            Logger::critical(d->m_coinKey, "Error call EthashAux::eval");
            continue;
          }
          if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
            break;
        }
      }
      break;
    }
    }
  }
  d->m_deviceHRCounter.reset();
}

bool EthOCLMiner::benchmark(qint64 &_elapsed, quint32 &_hashes) {
  Q_D(EthOCLMiner);
  if (!d->init())
      return false;

  quint32 groupSize; // localWork
  quint32 multiplier;
  {
    QMutexLocker l(&d->m_paramsMutex);
    if (!d->m_deviceParams.contains((int)OclDeviceParam::LocalWorkSize)) {
      groupSize = Settings::instance().clLocalWork(d->m_device, HashFamily::Ethereum);
      d->m_deviceParams.insert((int)OclDeviceParam::LocalWorkSize, groupSize);
    } else
      groupSize = d->m_deviceParams.value((int)OclDeviceParam::LocalWorkSize).value<quint32>();

    if (!d->m_deviceParams.contains((int)OclDeviceParam::GlobalWorkSize)) {
      multiplier = Settings::instance().clGlobalWork(d->m_device, HashFamily::Ethereum);
      d->m_deviceParams.insert((int)OclDeviceParam::GlobalWorkSize, multiplier);
    } else
      multiplier = d->m_deviceParams.value((int)OclDeviceParam::GlobalWorkSize).value<quint32>();
  }

  AlgoContextPtr algo; // scoped algo context
  DagController &dagCtl = DagController::instance();

  bool ok = false;
  try {
    ok = checkCardCapability(d->m_testJob);
  } catch (...) {
    return false;
  }

  if (!ok)
    return false;

  std::atomic<bool> inProgress(true);
  PreparingError error = PreparingError::NoError;
  if (!dagCtl.prepareTestDag(d->m_testJob.blob, inProgress, error))
    return false;
  const DagType &dag = dagCtl.dag();
  if (!dag) {
    qCritical() << "DAG not ready";
    return false;
  }

  auto setupAlgo = [&algo, d]() -> bool {
    algo.reset(new EthereumAlgoContext);
    return d->setupAlgo(algo);
  };

  if (!setupAlgo())
    return false;

  if (!d->setKernelArgs(algo, d->m_testJob))
    return false;

  d->m_hashCount = 0;
  volatile std::atomic<MinerThreadState> state(MinerThreadState::Mining);

  const quint8 SKIPPED_COUNT = 2; //skip N first tests
  const quint16 TEST_COUNT = 1000; // count of valid tests
  QElapsedTimer timer;
  qDebug() << "start testing...";
  NonceCtl *nonceCtl = NonceCtl::instance(d->m_coinKey, false);
  nonceCtl->restart(true);
  for (quint16 i = 0; i < TEST_COUNT + SKIPPED_COUNT; i++) {
    if (i == SKIPPED_COUNT)
      timer.start();

    QList<Nonce> results;
    try {
      if (!d->findShares(algo, results, state)) {
        d->m_manager->critical("Error call OpenCL program");
        return false;
      }
    } catch (...) {
      d->m_manager->critical("Error call find shares");
      return false;
    }

    _hashes += d->m_hashCount;
    d->m_hashCount = 0;
  }

  _elapsed = timer.elapsed();
  return true;
}

bool EthOCLMiner::allowed() const {
  return checkCardCapability();
}

GpuMinerError EthOCLMiner::checkCardCapability(const AbstractGPUManager *_manager, quint32 _device, const MiningJob &_job) {
  using namespace dev;
  using namespace dev::eth;

  const OCLDevicePtr &device = _manager->deviceProps(_device).value<OCLDevicePtr>();
  if (device->OCLVersion < 1.1) {
    Logger::warning(QString::null, QString("The device '%1' is not support the minimal version of OpenCL (1.1)").arg(device->deviceName));
    return GpuMinerError::OclVersionNotSupported;
  }
  quint64 requiredSize;

  // check available GPU memory size
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

  if (device->globalMemSize < requiredSize) {
    Logger::critical(QString::null, QString("[OpenCL] GPU memory size too small for working ETH miner (required %1 bytes, available %2 bytes)").arg(requiredSize, 0, 10).arg(device->globalMemSize, 0, 10));
    return GpuMinerError::NoMemory;
  }

  if (!_job.blob.isEmpty()) {
    // check for real required DAG size only
    if (device->maxMemAllocSize < requiredSize) {
      Logger::critical(QString::null, QString("[OpenCL] GPU maximal allocation memory size too small for working ETH miner (required %1 bytes, available %2 bytes)").arg(requiredSize, 0, 10).arg(device->maxMemAllocSize, 0, 10));
      return GpuMinerError::NoMemory;
    }
  }

  return GpuMinerError::NoError;
}

bool EthOCLMiner::checkCardCapability(const MiningJob &_job) const {
  Q_D(const EthOCLMiner);
  d->m_lastError = checkCardCapability(d->m_manager, d->m_device, _job);
  return GpuMinerError::NoError == d->m_lastError;
}

}
