#include "cryptonote/cryptonoteoclminer.h"

#include "openclminer/openclmanager.h"
#include "openclminer/openclminer_p.h"
#include "openclminer/cltypes.h"

#include "miner-abstract/noncectl.h"
#include "miner-abstract/miningjob.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/utilscommonprerequisites.h"

#include "hash.h"
#include "cl_cryptonight.h"

#ifdef QT_DEBUG
#include <QDebug>
#endif

namespace MinerGate {

const quint8 EXTRA_BUFFER_COUNT(6);
const quint8 KERNEL_COUNT(7);
const QString ERROR_SET_KERNEL_ARG("[%4] Error %1 when calling clSetKernelArg for kernel %2, argument %3.");

#define createExtraBuffer(a, b, c) { \
  algo->extraBuffers[a] = clCreateBuffer(algo->context, CL_MEM_READ_WRITE, b, NULL, &retval); \
  if(retval != CL_SUCCESS) { \
    Logger::critical(QString::null, QString(c).arg(retval)); \
    return false; \
  } \
}

enum ExtraBuffer : quint8 {BufScratchpads, BufStates, BufBlake256, BufGroestl256, BufJH256, BufSkein512};

struct CryptonoteAlgoContext: public AlgoContext {
  cl_mem inputBuffer = nullptr;
  cl_mem outputBuffer = nullptr;
  cl_mem extraBuffers[EXTRA_BUFFER_COUNT] = {};
  cl_kernel kernels[KERNEL_COUNT] = {};
  ~CryptonoteAlgoContext() {
    if (inputBuffer)
      clReleaseMemObject(inputBuffer);
    if (outputBuffer)
      clReleaseMemObject(outputBuffer);
    for (int i = 0; i < EXTRA_BUFFER_COUNT; i++)
      if (extraBuffers[i])
        clReleaseMemObject(extraBuffers[i]);
    for (int i = 0; i < KERNEL_COUNT; i++)
      if (kernels[i])
        clReleaseKernel(kernels[i]);
  }
};

class CryptonoteOCLMinerPrivate: public OpenCLMinerPrivate {
public:
  CryptonoteOCLMinerPrivate(quint32 _device, AbstractGPUManager *_manager):
    OpenCLMinerPrivate(_device, _manager) {
  }

  bool setupAlgo(const AlgoContextPtr &_algo) {
    const OCLDevicePtr &device = m_manager->deviceProps(m_device).value<OCLDevicePtr>();
    CryptonoteAlgoContext *algo = static_cast<CryptonoteAlgoContext *>(_algo.data());
    Q_ASSERT(algo);

    cl_int retval;
    std::string code(CL_CRYPTONIGHT, CL_CRYPTONIGHT + CL_CRYPTONIGHT_SIZE);
    const char *kernelSource = code.c_str();

    // create context for this device only
    algo->context = clCreateContext(NULL, 1, &device->deviceID, NULL, NULL, &retval);
    if (retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("Can't create device context: %1").arg(retval));
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
          groupSize = Settings::instance().clLocalWork(m_device, HashFamily::CryptoNight);
          m_deviceParams.insert((int)OclDeviceParam::LocalWorkSize, groupSize);
        }

        if (m_deviceParams.contains((int)OclDeviceParam::GlobalWorkSize))
          multiplier = m_deviceParams.value((int)OclDeviceParam::GlobalWorkSize).value<quint32>();
        else {
          multiplier = Settings::instance().clGlobalWork(m_device, HashFamily::CryptoNight);
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


    const cl_queue_properties commandQueueProperties[] = { 0, 0, 0 };
    size_t globalThreads = algo->globalWorkSize;
    size_t localThreads = algo->workgroupSize;
    Q_ASSERT(globalThreads);
    Q_ASSERT(localThreads);
    algo->commandQueues = clCreateCommandQueueWithProperties(algo->context, device->deviceID, commandQueueProperties, &retval);
    if (retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[1] OpenCL error %1").arg(retval));
      return false;
    }
    algo->inputBuffer = clCreateBuffer(algo->context, CL_MEM_READ_ONLY, 80, NULL, &retval);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[2] OpenCL error %1").arg(retval));
      return false;
    }
    // (1 << 21) is 2 Mb
    createExtraBuffer(BufScratchpads, (1 << 21) * globalThreads, "[3] OpenCL error %1");
    createExtraBuffer(BufStates, 200 * globalThreads, "[4] OpenCL error %1");
    createExtraBuffer(BufBlake256, sizeof(cl_uint) * (globalThreads + 2), "[5] OpenCL error %1");
    createExtraBuffer(BufGroestl256, sizeof(cl_uint) * (globalThreads + 2), "[6] OpenCL error %1");
    createExtraBuffer(BufJH256, sizeof(cl_uint) * (globalThreads + 2), "[7] OpenCL error %1");
    createExtraBuffer(BufSkein512, sizeof(cl_uint) * (globalThreads + 2), "[8] OpenCL error %1");

    algo->outputBuffer = clCreateBuffer(algo->context, CL_MEM_READ_WRITE, sizeof(cl_uint) * 0x100, NULL, &retval);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[9] OpenCL error %1").arg(retval));
      return false;
    }

    algo->program = clCreateProgramWithSource(algo->context, 1, &kernelSource, NULL, &retval);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[10] OpenCL error %1").arg(retval));
      return false;
    }

    const char *options = QString("-I. -DWORKSIZE=%1").arg(localThreads).toLatin1().constData();
    retval = clBuildProgram(algo->program, 1, &device->deviceID, options, NULL, NULL);
    size_t len;
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[11] OpenCL error %1").arg(retval));
      retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);

      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[12] OpenCL error %1").arg(retval));
        return false;
      }

      QScopedPointer<char> buildLog((char *)malloc(sizeof(char) * (len + 2)));
      retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, len, buildLog.data(), NULL);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[13] OpenCL error %1").arg(retval));
        return false;
      }

      Logger::critical(QString::null, QString("Compile error, build log:\n%1").arg(buildLog.data()));
      return false;
    } else
      Logger::debug(QString::null, "compile ok");

    cl_build_status status;
    do {
      retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[14] OpenCL error %1").arg(retval));
        QThread::sleep(1);
        continue;
      }
    } while(status == CL_BUILD_IN_PROGRESS);

    retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[15] OpenCL error %1").arg(retval));
      return false;
    }

    QScopedPointer<char> buildLog((char *)malloc(sizeof(char) * (len + 2)));
    retval = clGetProgramBuildInfo(algo->program, device->deviceID, CL_PROGRAM_BUILD_LOG, len, buildLog.data(), NULL);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[16] OpenCL error %1").arg(retval));
      return false;
    }

    Logger::debug(QString::null, QString("build Log:\n%1").arg(buildLog.data()));

    const QStringList kernelNames = { "cn0", "cn1", "cn2", "Blake", "Groestl", "JH", "Skein" };
    Q_ASSERT(kernelNames.count() == KERNEL_COUNT);
    for(int i = 0; i < kernelNames.count(); ++i) {
      const QString &kName = kernelNames.at(i);
      algo->kernels[i] = clCreateKernel(algo->program, kName.toLocal8Bit().constData(), &retval);
      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[17] OpenCL error %1, kernel: %2.").arg(retval).arg(kernelNames[i]));
        return false;
      }
    }

    Logger::info(QString::null, "cl programm building complete");

    return true;
  }

  bool setKernelArgs(const AlgoContextPtr &_algo, const MiningJob &_job) {
    CryptonoteAlgoContext *algo = static_cast<CryptonoteAlgoContext *>(_algo.data());
    if (!algo)
      return false;

    cl_int retval;

    if(!algo || _job.blob.isEmpty())
      return false;

    retval = clEnqueueWriteBuffer(algo->commandQueues, algo->inputBuffer, CL_TRUE, 0, 76, _job.blob.constData(), 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      m_manager->critical(QString("[29] OpenCL error %1").arg(retval));
      return false;
    }

    retval = clSetKernelArg(algo->kernels[0], 0, sizeof(cl_mem), &algo->inputBuffer);
    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(0).arg(0).arg(30));
      return false;
    }

    // Scratchpads
    retval = clSetKernelArg(algo->kernels[0], 1, sizeof(cl_mem), &algo->extraBuffers[0]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(0).arg(1).arg(31));
      return false;
    }

    // States
    retval = clSetKernelArg(algo->kernels[0], 2, sizeof(cl_mem), &algo->extraBuffers[1]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(0).arg(2).arg(32));
      return false;
    }

    // CN2 Kernel

    // Scratchpads
    retval = clSetKernelArg(algo->kernels[1], 0, sizeof(cl_mem), &algo->extraBuffers[0]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(1).arg(0).arg(33));
      return false;
    }

    // States
    retval = clSetKernelArg(algo->kernels[1], 1, sizeof(cl_mem), &algo->extraBuffers[1]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(1).arg(1).arg(34));
      return false;
    }

    // CN3 Kernel
    // Scratchpads
    retval = clSetKernelArg(algo->kernels[2], 0, sizeof(cl_mem), &algo->extraBuffers[0]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(0).arg(35));
      return false;
    }

    // States
    retval = clSetKernelArg(algo->kernels[2], 1, sizeof(cl_mem), &algo->extraBuffers[1]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(1).arg(36));
      return false;
    }

    // Branch 0
    retval = clSetKernelArg(algo->kernels[2], 2, sizeof(cl_mem), &algo->extraBuffers[2]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(2).arg(37));
      return false;
    }

    // Branch 1
    retval = clSetKernelArg(algo->kernels[2], 3, sizeof(cl_mem), &algo->extraBuffers[3]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(3).arg(38));
      return false;
    }

    // Branch 2
    retval = clSetKernelArg(algo->kernels[2], 4, sizeof(cl_mem), &algo->extraBuffers[4]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(4).arg(39));
      return false;
    }

    // Branch 3
    retval = clSetKernelArg(algo->kernels[2], 5, sizeof(cl_mem), &algo->extraBuffers[5]);

    if(retval != CL_SUCCESS) {
      m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(2).arg(5).arg(40));
      return false;
    }

    for(int i = 0; i < 4; ++i) {
      // States
      retval = clSetKernelArg(algo->kernels[i + 3], 0, sizeof(cl_mem), &algo->extraBuffers[1]);

      if(retval != CL_SUCCESS) {
        m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(i + 3).arg(0).arg(41));
        return false;
      }

      // Nonce buffer
      retval = clSetKernelArg(algo->kernels[i + 3], 1, sizeof(cl_mem), &algo->extraBuffers[i + 2]);

      if(retval != CL_SUCCESS) {
        m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(i + 3).arg(1).arg(42));
        return false;
      }

      // Output
      retval = clSetKernelArg(algo->kernels[i + 3], 2, sizeof(cl_mem), &algo->outputBuffer);

      if(retval != CL_SUCCESS) {
        m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(i + 3).arg(2).arg(43));
        return false;
      }

      // Target
      retval = clSetKernelArg(algo->kernels[i + 3], 3, sizeof(cl_uint), &_job.target);

      if(retval != CL_SUCCESS) {
        m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(i + 3).arg(3).arg(44));
        return false;
      }
    }
    return true;
  }

  bool findShares(const AlgoContextPtr &_algo, void *hashOutput, bool isFee, volatile std::atomic<MinerThreadState> &_state){
    const OCLDevicePtr &device = m_manager->deviceProps(m_device).value<OCLDevicePtr>();
    CryptonoteAlgoContext *algo = static_cast<CryptonoteAlgoContext *>(_algo.data());
    if (!algo)
      return false;

    QElapsedTimer timer;
    timer.start();
    cl_int retval;
    cl_uint zero = 0;
    size_t globalThreads = algo->globalWorkSize, localThreads = algo->workgroupSize;
    size_t branchNonces[4] = { 0 };

    if(!algo || !hashOutput)
      return false;

    for(int i = 2; i < 6; ++i) {
      retval = clEnqueueWriteBuffer(algo->commandQueues, algo->extraBuffers[i], CL_FALSE, sizeof(cl_uint) * globalThreads, sizeof(cl_uint), &zero, 0, NULL, NULL);

      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[18] OpenCL error %1, kernel: %2.").arg(retval).arg(i - 2));
        return false;
      }
    }

    retval = clEnqueueWriteBuffer(algo->commandQueues, algo->outputBuffer, CL_FALSE, sizeof(cl_uint) * 0xFF, sizeof(cl_uint), &zero, 0, NULL, NULL);
    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[19] OpenCL error %1").arg(retval));
      return false;
    }
    clFinish(algo->commandQueues);

    size_t finishNonce;
    size_t startNonce;

    NonceCtl *nonceCtl = NonceCtl::instance(m_coinKey, isFee);
    while (_state == MinerThreadState::Mining)
      if (!nonceCtl->isValid())
        QThread::msleep(100);
      else {
        finishNonce = nonceCtl->nextNonce(globalThreads);
        break;
      }
    startNonce = finishNonce - globalThreads + 1;

    size_t nonce[2] = {startNonce, 1};
    size_t gthreads[2] = { globalThreads, 8 };
    size_t lthreads[2] = { localThreads, 8 };

    {
      retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->kernels[0], 2, nonce, gthreads, lthreads, 0, NULL, NULL);

      if(retval != CL_SUCCESS) {
        Logger::critical(QString::null, QString("[20] OpenCL error %1, kernel: %2.").arg(retval).arg(0));
        return false;
      }
    }

    retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->kernels[1], 1, &startNonce, &globalThreads, &localThreads, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[21] OpenCL error %1, kernel: %2.").arg(retval).arg(1));
      return false;
    }

    retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->kernels[2], 2, nonce, gthreads, lthreads, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[22] OpenCL error %1, kernel: %2.").arg(retval).arg(2));
      return false;
    }

    retval = clEnqueueReadBuffer(algo->commandQueues, algo->extraBuffers[2], CL_FALSE, sizeof(cl_uint) * globalThreads, sizeof(cl_uint), branchNonces, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[23] OpenCL error %1").arg(retval));
      return false;
    }

    retval = clEnqueueReadBuffer(algo->commandQueues, algo->extraBuffers[3], CL_FALSE, sizeof(cl_uint) * globalThreads, sizeof(cl_uint), branchNonces + 1, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[24] OpenCL error %1").arg(retval));
      return false;
    }

    retval = clEnqueueReadBuffer(algo->commandQueues, algo->extraBuffers[4], CL_FALSE, sizeof(cl_uint) * globalThreads, sizeof(cl_uint), branchNonces + 2, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[25] OpenCL error %1").arg(retval));
      return false;
    }

    retval = clEnqueueReadBuffer(algo->commandQueues, algo->extraBuffers[5], CL_FALSE, sizeof(cl_uint) * globalThreads, sizeof(cl_uint), branchNonces + 3, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[26] OpenCL error %1").arg(retval));
      return false;
    }

    clFinish(algo->commandQueues);

    for(int i = 0; i < 4; ++i) {
      if(branchNonces[i]) {

        retval = clSetKernelArg(algo->kernels[i + 3], 4, sizeof(cl_ulong), &branchNonces + i);
        branchNonces[i] += branchNonces[i] + (localThreads - (branchNonces[i] & (localThreads - 1)));

        if(retval != CL_SUCCESS) {
          m_manager->critical(ERROR_SET_KERNEL_ARG.arg(retval).arg(-1).arg(i + 3).arg(-1));
          return false;
        }


        if (device->OCLVersion >= 2) {
          size_t localWorkSize = branchNonces[i] * 1;
          retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->kernels[i + 3], 1, &startNonce, branchNonces + i, &localWorkSize, 0, NULL, NULL);
        } else
          retval = clEnqueueNDRangeKernel(algo->commandQueues, algo->kernels[i + 3], 1, &startNonce, branchNonces + i, NULL, 0, NULL, NULL);

        if(retval != CL_SUCCESS) {
          Logger::critical(QString::null, QString("[27] OpenCL error %1, kernel: %2.").arg(retval).arg(i + 3));
          return false;
        }
      }
    }

    retval = clEnqueueReadBuffer(algo->commandQueues, algo->outputBuffer, CL_TRUE, 0, sizeof(cl_uint) * 0x100, hashOutput, 0, NULL, NULL);

    if(retval != CL_SUCCESS) {
      Logger::critical(QString::null, QString("[28] OpenCL error %1").arg(retval));
      return false;
    }

    clFinish(algo->commandQueues);
    return true;
  }
};

CryptonoteOCLMiner::CryptonoteOCLMiner(quint32 _device, AbstractGPUManager *_manager):
  OpenCLMiner(*new CryptonoteOCLMinerPrivate(_device, _manager)) {
}

CryptonoteOCLMiner::~CryptonoteOCLMiner() {
}

void CryptonoteOCLMiner::start(MiningJob &_job, MiningJob &_feeJob, QMutex &_jobMutex, QMutex &_feeJobMutex,
                        volatile std::atomic<MinerThreadState> &_state, bool _fee) {
  Q_D(CryptonoteOCLMiner);
  if (!d->init())
    return;
  AlgoContextPtr algo; // scoped algo context
  d->m_hashCount = 0;

  auto setupAlgo = [&algo, d]() -> bool {
    algo.reset(new CryptonoteAlgoContext);
    return d->setupAlgo(algo);
  };

  if (!setupAlgo()) {
    Logger::critical(QString::null, "Can't setup algorythm");
    return;
  }

  MiningJob job;
  quint32 target[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0};
  enum JobKind {KindUnknown, KindNormal, KindFee};
  JobKind lastJobKind = KindUnknown;

  if (!d->setKernelArgs(algo, _job))
    return;

  bool algoReady = false;
  Q_FOREVER {
    switch(_state) {
    case MinerThreadState::Exit:
    case MinerThreadState::Stopping:
      d->m_hashCount = 0;
      return;
    case MinerThreadState::UpdateJob: {
      bool empty;
      {
        QMutexLocker lock(&_jobMutex);
        empty = _job.job_id.isEmpty();
      }
      if (empty) {
        QThread::sleep(3);
        continue;
      } else {
        lastJobKind = KindUnknown;
        if (_state == MinerThreadState::UpdateJob)
          _state = MinerThreadState::Mining;
      }
      break;
    }
    case MinerThreadState::Mining: {

      if (!algoReady) {
        if (!setupAlgo()) {
          _state = MinerThreadState::Exit;
          return;
        }
        algoReady = true;
      }

      bool isFee = _fee && qrand() % 100 < 2;
      if(isFee) {
        if (lastJobKind != KindFee) {
          QMutexLocker lock(&_feeJobMutex);
          job = _feeJob;
          lastJobKind = KindFee;
        }
      } else {
        if (lastJobKind != KindNormal) {
          QMutexLocker lock(&_jobMutex);
          job = _job;
          lastJobKind = KindNormal;
        }
      }

      target[7] = arrayToQuint32(job.target);
      if (!d->setKernelArgs(algo, job)) {
        Logger::critical(QString::null, "Can't set kernel arguments");
        return;
      }

      cl_uint results[0x100] = { 0 };
      if (!d->findShares(algo, results, isFee, _state)) {
        d->m_manager->critical("Error call OpenCL program");
        _state = MinerThreadState::Exit;
        return;
      }
      if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
        continue;
      d->m_hashCount += algo->globalWorkSize;

      // at address results[0xFF] - count of found hashes
      for(int i = 0; i < results[0xFF]; ++i) {
        quint32 foundNonce = results[i];
        d->m_manager->info(QString("AMD share found, nonce = %1").arg(foundNonce));
        // check on CPU and get share
        QByteArray tempdata(job.blob);
        std::memcpy(&tempdata.data()[39], &foundNonce, sizeof(foundNonce));
        crypto::cn_context context;
        crypto::hash hash;
        std::memset(&hash, 0, sizeof(hash));

        crypto::cn_slow_hash(context, tempdata.data(), tempdata.size(), hash);
        if(((quint32*)&hash)[7] <= arrayToQuint32(job.target)) {
          const QByteArray hashArray(reinterpret_cast<const char*>(&hash), sizeof(hash));
          Q_EMIT shareFound(job.job_id, hashArray, foundNonce, isFee);
        }
        else
          fprintf(stderr, "GPU #%d: result for nonce $%08X does not validate on CPU!\n", d->m_device, foundNonce);
      }
      if (_state == MinerThreadState::Exit || _state == MinerThreadState::Stopping)
        continue;
    }
      break;
    }
  }
  d->m_deviceHRCounter.reset();
}

bool CryptonoteOCLMiner::benchmark(qint64 &_elapsed, quint32 &_hr) {
  _elapsed = 0;
  _hr = 0u;
  Q_D(CryptonoteOCLMiner);
  AlgoContextPtr algo; // scoped algo context

  auto setupAlgo = [&algo, d]() -> bool {
    algo.reset(new CryptonoteAlgoContext);
    return d->setupAlgo(algo);
  };

  if (!setupAlgo()) {
    Logger::critical(QString::null, "Can't setup algorythm");
    return false;
  }

  const quint8 BLOB_SIZE(152u);
  MiningJob job;

  const quint8 TEST_COUNT(3u);
  QElapsedTimer timer;
  timer.start();
  for (quint8 i = 0u; i < TEST_COUNT; i++) {
    job.blob.fill(i + 1, BLOB_SIZE);
    job.target = quint32ToArray(1u);
    if (!d->setKernelArgs(algo, job)) {
      Logger::critical(QString::null, "Can't set kernel arguments");
      return false;
    }
    cl_uint results[0x100] = { 0u };
    volatile std::atomic<MinerThreadState> stateStub(MinerThreadState::Mining);
    if (!d->findShares(algo, results, false, stateStub)) {
      d->m_manager->critical("Error call OpenCL program");
      return false;
    }
    _hr += algo->globalWorkSize;
  }
  _elapsed = timer.elapsed();
  _hr /= TEST_COUNT;
  return true;
}

GpuMinerError CryptonoteOCLMiner::checkCardCapability(const AbstractGPUManager *_manager, quint32 _device) {
  const OCLDevicePtr &device = _manager->deviceProps(_device).value<OCLDevicePtr>();
  if (device->OCLVersion < 1.2)
    return GpuMinerError::OclVersionNotSupported;
  return GpuMinerError::NoError;
}

bool CryptonoteOCLMiner::allowed() const {
  Q_D(const CryptonoteOCLMiner);
  return checkCardCapability(d->m_manager, d->m_device) == GpuMinerError::NoError;
}

}
