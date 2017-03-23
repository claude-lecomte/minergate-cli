#include "openclminer/openclmanager.h"
#include "cryptonote/cryptonoteoclminer.h"
#ifndef NO_ETH
#include "ethereum/ethoclminer.h"
#endif

#include "miner-abstract/abstractgpumanager_p.h"

#include "utils/logger.h"
#include "utils/settings.h"
#include "utils/familyinfo.h"

#include "openclminer/cltypes.h"

#include <QFile>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <QThread>
#include <QtMath>

namespace MinerGate {

class OpenCLManagerPrivate: public AbstractGPUManagerPrivate {
  Q_DECLARE_PUBLIC(OpenCLManager)
public:
  OpenCLManager *q_ptr;

  OpenCLManagerPrivate(OpenCLManager *_q):
    AbstractGPUManagerPrivate(ImplType::OpenCL), q_ptr(_q) {
    m_devicesMap = openCLDevices();
  }

  QScopedPointer<cl_device_id> m_devices;
  cl_uint m_numDevices = 0;

private:
  QMap<quint32, QVariant> openCLDevices() throw() {
    Q_Q(OpenCLManager);
    QMap<quint32, QVariant> res;

    cl_uint numPlatforms;
    cl_platform_id platform = nullptr;
    cl_int	status = CL_SUCCESS;
    try {
      status = clGetPlatformIDs(0, NULL, &numPlatforms);
    } catch (...) {
      Logger::critical(QString::null, "Can't call clGetPlatformIDs");
      return res;
    }

    if (status != CL_SUCCESS) {
      Logger::debug(QString::null, QString("Can't get OpenCL platforms! Error: %1").arg(status));
      return res;
    }
    if(!numPlatforms) {
      Logger::info(QString::null, QString("No OpenCL-compatible platforms"));
      return res;
    }
    Logger::info(QString::null, QString("number of OpenCL platform: %1").arg(numPlatforms));
    {
      cl_platform_id* platforms = static_cast<cl_platform_id*>(malloc(numPlatforms* sizeof(cl_platform_id)));
      try {
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetPlatformIDs");
        free(platforms);
        return res;
      }
      // find AMD and Clover platform only
      size_t len;
      for (int i = 0; i < numPlatforms; i++) {
        try {
          if (CL_SUCCESS != clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &len)) {
            Logger::critical(QString::null, "Can't query platform name");
            continue;
          }
        } catch (...) {
          Logger::critical(QString::null, "Can't call clGetPlatformInfo");
          free(platforms);
          return res;
        }
        char *platformName = static_cast<char *>(malloc(sizeof(char) * (len + 2)));
        try {
          if (CL_SUCCESS != clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, len, platformName, NULL)) {
            Logger::critical(QString::null, "[2] Can't query platform name");
            continue;
          }
        } catch (...) {
          Logger::critical(QString::null, "Can't call clGetPlatformInfo");
          free(platformName);
          free(platforms);
          return res;
        }
        bool skip = !QString(platformName).contains("AMD") && !QString(platformName).contains("Clover");
        Logger::info(QString::null, QString("found OpenCL platform: %1: %2").arg(platformName).arg(skip?"skipped":"will use"));
        if (!skip)
            platform = platforms[i];
        free(platformName);
      }
      free(platforms);
    }

    if (!platform) {
      Logger::info(QString::null, "no available OpenCL found");
      return res;
    }

    try {
      status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &m_numDevices);
    } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceIDs");
        return res;
    }
    if (!m_numDevices) {
      Logger::info(QString::null, "No OpenCL GPU available");
      return res;
    } else {
      m_devices.reset(static_cast<cl_device_id*>(malloc(m_numDevices * sizeof(cl_device_id))));
      try {
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, m_numDevices, m_devices.data(), NULL);
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceIDs");
        return res;
      }
    }

    // create temporary OpenCL context
    cl_context localContext;
    try {
      localContext = clCreateContext(NULL, m_numDevices, m_devices.data(), NULL, NULL, NULL);
    } catch (...) {
      Logger::critical(QString::null, "Can't call clCreateContext");
      return res;
    }

    for (int i = 0; i < m_numDevices; i++) {
      const cl_device_id &devId = m_devices.data()[i];
      OCLDevicePtr device(new OCLDevice);
      device->deviceID = devId;
      device->platformId = 0;
      // get device info
      size_t len;
      try {
        if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_NAME, 0, NULL, &len)) {
          Logger::critical(QString::null, "Can't query device name");
          continue;
        }
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceInfo");
        return res;
      }

      char *devName = static_cast<char *>(malloc(sizeof(char) * (len + 2)));
      try {
        if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_NAME, len, devName, NULL)) {
          Logger::critical(QString::null, "[2] Can't get device name");
          continue;
        }
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceInfo");
        return res;
      }

      device->deviceName = devName;
      Logger::info(QString::null, QString("Found OpenCL device: %1").arg(devName));

      try {
        if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_VERSION, 0, NULL, &len)) {
          Logger::critical(QString::null, "Can't get device version");
          continue;
        }
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceInfo");
        return res;
      }

      char *version = static_cast<char *>(malloc(sizeof(char) * (len + 2)));
      try {
        if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_VERSION, len, version, NULL)) {
          Logger::critical(QString::null, "[2] Can't get device version");
          continue;
        }
      } catch (...) {
        Logger::critical(QString::null, "Can't call clGetDeviceInfo");
        return res;
      }

      int idx = 7;

      // 0x20 == SPACE
      while(version[idx++] != 0x20 && idx < strlen(version));

      if(idx == strlen(version)) {
        Logger::critical(QString::null, "Error get version");
        continue;
      }

      // NULL terminate the string here
      version[idx - 1] = 0x00;

      QStringList oclParams = QString(version).split(' ');
      free(version);

      if (oclParams.count() < 2) {
        Logger::critical(QString::null, QString("Can't get OpenCL version for device: %1").arg(devName));
        continue;
      }
      device->OCLVersion = oclParams.at(1).toDouble();
      Logger::info(QString::null, QString("OCL version: %1").arg(device->OCLVersion));

      if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &device->maximumWorkGroupSize, NULL)) {
        Logger::critical(QString::null, "Error querying a device's max worksize");
        continue;
      }
      Logger::info(QString::null, QString("OCL maximum worksize is %1").arg(device->maximumWorkGroupSize));

      if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(size_t), &device->globalMemSize, NULL)) {
        Logger::critical(QString::null, "Error querying a device's global memory size");
        continue;
      }
      Logger::info(QString::null, QString("OCL global memory size is %1").arg(device->globalMemSize));

      if (CL_SUCCESS != clGetDeviceInfo(devId, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(size_t), &device->maxMemAllocSize, NULL)) {
        Logger::critical(QString::null, "Error querying a device's maximal memory allocation size for one object");
        continue;
      }
      Logger::info(QString::null, QString("OCL maximal allocation memory size is %1").arg(device->maxMemAllocSize));

      // all ok - add device to work
      res.insert(i, qVariantFromValue(device));
    }
    clReleaseContext(localContext);
    return res;
  }
};

OpenCLManager::OpenCLManager():
  AbstractGPUManager(*new OpenCLManagerPrivate(this)) {
}

QString OpenCLManager::deviceName(quint32 _device) const {
  Q_D(const OpenCLManager);
  const QVariant &val = d->m_devicesMap.value(_device);
  if (val.isNull())
    return QString();
  return val.value<OCLDevicePtr>()->deviceName;
}

GPUCoinPtr OpenCLManager::createMiner(quint32 _device, HashFamily _family)  {
  switch (_family) {
  case HashFamily::CryptoNight:
    return GPUCoinPtr(new CryptonoteOCLMiner(_device, this));
#ifndef NO_ETH
  case HashFamily::Ethereum:
    return GPUCoinPtr(new EthOCLMiner(_device, this));
#endif
  default: return GPUCoinPtr();
  }
}

bool OpenCLManager::checkDeviceAvailable(quint32 _device, HashFamily _family) const {
  switch (_family) {
  case HashFamily::CryptoNight:
    return CryptonoteOCLMiner::checkCardCapability(this, _device) == GpuMinerError::NoError;
#ifndef NO_ETH
  case HashFamily::Ethereum:
    return EthOCLMiner::checkCardCapability(this, _device) == GpuMinerError::NoError;
#endif
  default: return false;
  }
}

}


