#pragma once

#include <QVariant>
#include <QString>
#include <QScopedPointer>

#include "miner-abstract/minerabstractcommonprerequisites.h"

namespace MinerGate {

enum class HashFamily: quint8;
class AbstractGPUManagerPrivate;
class AbstractGPUManager: public QObject {
  Q_OBJECT
  Q_DECLARE_PRIVATE(AbstractGPUManager)
  Q_DISABLE_COPY(AbstractGPUManager)
public:
  enum MessageType {Info, Critical, Debug};

  ~AbstractGPUManager();
  quint8 deviceCount() const;
  quint8 deviceCount(HashFamily _family) const;
  QVariant deviceProps(quint32 _device) const;
  static quint32 deviceNo(const QString &deviceKey);
  virtual bool checkDeviceAvailable(quint32 _device, HashFamily _family) const = 0;
  GPUCoinPtr getMiner(quint32 _device, HashFamily _family, bool create = true);
  void freeMiner(quint32 _device);
  void message(MessageType _type, const QString &_msg);
  virtual QString deviceName(quint32 _device) const = 0;
  QString deviceKey(quint32 _device) const;
  ImplType type() const;
  bool availableForFamily(HashFamily _family) const;
Q_SIGNALS:
  void info(const QString &_msg);
  void critical(const QString &_msg);
  void debug(const QString &_msg);
protected:
  QScopedPointer<AbstractGPUManagerPrivate> d_ptr;

  AbstractGPUManager(AbstractGPUManagerPrivate &d);
  virtual GPUCoinPtr createMiner(quint32 _device, HashFamily family) = 0;
};

class GPUManagerPtr: public QSharedPointer<AbstractGPUManager> {
};

}
