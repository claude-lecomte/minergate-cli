#pragma once

#include <QEvent>
#include <QString>
#include <QSharedPointer>

class QThread;

namespace MinerGate {

class AbstractMinerImpl;

enum class CryptonotePoolError : qint32 {
  NoError = 0,
  WrongParam = -1,
  InvalidShareData = -13,
  InvalidJobId = -14,
  UnknownJobShare = -15,
  SlightlyStaleShare = -16,
  ReallyStaleShare = -17,
  DuplicateShare = -18,
  InvalidShare = -19,
  CanGetJob = -20,
  UnknownWorker = -21,
  UnknownWorkerSession = -22,
  InvalidWorkerSessionId = -23,
  UnknownError = -24,
  WorkerSessionMismatch = -25,
  // additional ethereum poll errors:
  StaleJob = 21,
  LowDifficultyShare = CryptonotePoolError::InvalidWorkerSessionId
};

enum class EthereumPoolError : qint32 {
  NoError = 0,
  InvalidShare = 20,
  StaleJob = 21,
  StaleJob2 = -21,
  DuplicateShare = -22,
  LowDifficultyShare = -23,
  UnauthorizedWorker = -24,
  Timeout = -42,
  CantGenerateJob = -32002
  // additional ethereum poll errors:
};

enum class PoolState : quint8 {Connected, Error};
enum class MinerImplState : quint8 {Running, Starting, Stopped, Stopping, Error};
enum class CPUWorkerType : quint8 {Cryptonote, Ethereum};

typedef QSharedPointer<QThread> QThreadPtr;
}

Q_DECLARE_METATYPE(MinerGate::MinerImplState)
Q_DECLARE_METATYPE(MinerGate::PoolState)
