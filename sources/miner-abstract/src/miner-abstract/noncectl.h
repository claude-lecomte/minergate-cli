#pragma once

#include <QtCore>
#include "minerabstractcommonprerequisites.h"

class QString;
namespace MinerGate {

class NonceCtlPrivate;

class NonceCtl {
public:
  static NonceCtl *instance(const QString &_coin, bool _fee);
  Nonce nextNonce(Nonce reserve) const;
  void invalidate();
  bool isValid() const;
  void restart(bool randomStart);

  virtual ~NonceCtl();
private:
  NonceCtl();
  NonceCtlPrivate *d_ptr;
};

}
