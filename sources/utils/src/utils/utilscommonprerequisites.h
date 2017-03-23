#pragma once
#include <QSharedPointer>

class QTextStream;
class QFile;

namespace MinerGate
{
  typedef QSharedPointer<QTextStream> TextStreamPtr;
  typedef QSharedPointer<QFile> FilePtr;
  enum class HashFamily : quint8 { Unknown, CryptoNight, CryptoNightLite, Ethereum, First = HashFamily::CryptoNight, Last = HashFamily::Ethereum};

  inline uint qHash(const HashFamily &_family, uint _seed = 0) {
    return ::qHash((uint)_family, _seed);
  }

  inline const HashFamily operator ++(HashFamily& a) {
    a = static_cast<HashFamily>(static_cast<int>(a) + 1);
    return a;
  }
}

Q_DECLARE_METATYPE(MinerGate::HashFamily)
