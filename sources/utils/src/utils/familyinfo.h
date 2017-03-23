#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QMetaType>
#include <QHash>

#include "utilscommonprerequisites.h"

namespace MinerGate {

const QVector<HashFamily> CPUAvailable {HashFamily::CryptoNight, HashFamily::CryptoNightLite, HashFamily::Ethereum};
const QVector<HashFamily> GPUAvailable {HashFamily::CryptoNight/*,HashFamily::Scrypt, */, HashFamily::Ethereum};

class FamilyInfo {
public:
  static void add(const QString &_coinKey, HashFamily _family);
  static HashFamily familyByCoin(const QString &_coinKey);
  static void clear();
private:
  static QMap<QString, HashFamily> m_map;
};

}
