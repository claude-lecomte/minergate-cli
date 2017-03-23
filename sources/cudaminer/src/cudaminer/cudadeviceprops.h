#pragma once

#include <QSharedPointer>
#include <QVariant>
#include "cuda_runtime.h"

namespace MinerGate {

class CudaDevicePropPtr: public QSharedPointer<cudaDeviceProp> {
public:
  CudaDevicePropPtr():
    QSharedPointer<cudaDeviceProp>() {
  }

  CudaDevicePropPtr(const CudaDevicePropPtr &other):
    QSharedPointer<cudaDeviceProp>(other) {
  }

  CudaDevicePropPtr(cudaDeviceProp *props):
    QSharedPointer<cudaDeviceProp>(props) {
  }

  CudaDevicePropPtr(const QVariant &props):
    CudaDevicePropPtr(props.value<CudaDevicePropPtr>()) {
  }
};

}

Q_DECLARE_METATYPE(MinerGate::CudaDevicePropPtr)
