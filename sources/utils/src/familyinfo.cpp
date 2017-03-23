#include "utils/familyinfo.h"

namespace MinerGate {

QMap<QString, HashFamily> FamilyInfo::m_map;

void FamilyInfo::add(const QString &_coinKey, HashFamily _family) {
  m_map.insert(_coinKey, _family); 
}

HashFamily FamilyInfo::familyByCoin(const QString &_coinKey) {
  return m_map.value(_coinKey);
}

void FamilyInfo::clear() {
  m_map.clear();
}

}
