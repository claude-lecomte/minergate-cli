#include "gputester.h"
#include <cudaminer/cudamanager.h>
#include <cudaminer/cudaminer.h>

#include <openclminer/openclmanager.h>
#include <openclminer/openclminer.h>

#include <utils/utilscommonprerequisites.h>

#include <ethutils/dagcontroller.h>
#include <ethutils/testdata.h>

#include <QDebug>

namespace MinerGate {

class GPUTesterPrivate {
public:
  ImplType type;
  quint32 firstParam;
  quint32 secondParam;
  quint32 deviceNo;
  HashFamily family;

  GPUTesterPrivate (ImplType _type, HashFamily _family, quint32 _deviceNo, quint32 _firstParam, quint32 _secondParam):
    type(_type), family(_family), deviceNo(_deviceNo), firstParam(_firstParam), secondParam(_secondParam) {
  }

  bool test(quint32 &result, QString &_error) {
    _error = QString::null;
    result = 0;
    quint32 hashes = 0;
    GPUManagerPtr man;

    // create manager
    if (type == ImplType::CUDA)
      man.reset(new CudaManager);
    else if (type == ImplType::OpenCL)
      man.reset(new OpenCLManager);
    else {
      _error = "no_impl";
      return false;
    }

    if (!man->deviceCount(family)) {
      _error = "no_devices";
      return false;
    }

    // get miner for testing
    try {
      const GPUCoinPtr &miner = man->getMiner(deviceNo, family);
      if (!miner) {
        _error = "no_miner";
        return false;
      }
      if (!miner->allowed()) {
        _error = "miner_not_allowed";
        return false;
      }
      QMap<int, QVariant> gpuParams;
      if (type == ImplType::CUDA) {
        gpuParams.insert((int)CudaDeviceParam::Intensity, 4);
        gpuParams.insert((int)CudaDeviceParam::GridSize, firstParam);
        gpuParams.insert((int)CudaDeviceParam::BlockSize, secondParam);
      } else if (type == ImplType::OpenCL) {
        gpuParams.insert((int)OclDeviceParam::GlobalWorkSize, firstParam);
        gpuParams.insert((int)OclDeviceParam::LocalWorkSize, secondParam);
      }
      miner->setGpuParams(gpuParams);

      if (family == HashFamily::Ethereum) {
        // First, check whether the DAG is present
        if (!DagController::instance().testDagExists()) {
          _error = "DAG";
          return false;
        }
        MiningJob testJob(QByteArray::fromHex(TEST_HEADER_HASH.toLocal8Bit()),
                          QByteArray::fromHex(TEST_SEED_HASH.toLocal8Bit()),
                          QByteArray::fromHex(TEST_BOUDARY.toLocal8Bit()));
        miner->setCoinKey("eth");
        miner->setTestJob(testJob);
      } else
        miner->setCoinKey("bcn");

      qint64 elapsed = 0;
      if (!miner->benchmark(elapsed, hashes)) {
        _error = "becnhmark_failed";
        return false;
      }
      if (!elapsed || !hashes) {
        _error = "no_results";
        return false;
      }
      result = hashes / (elapsed / 1000.0);
    } catch (...) {
      _error = "unknown error";
      return false;
    }
    return true;
  }
};

GPUTester::~GPUTester() {
}

GPUTester::GPUTester(ImplType _type, HashFamily _family, quint32 _deviceNo, quint32 _firstParam, quint32 _secondParam):
  d_ptr(new GPUTesterPrivate(_type, _family, _deviceNo, _firstParam, _secondParam)) {
}

bool GPUTester::run(quint32 &result, QString &_error) {
  Q_D(GPUTester);
  return d->test(result, _error);
}

void GPUTester::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  // stub for all libs message
}

}
