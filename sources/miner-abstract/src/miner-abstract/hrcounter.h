#include <QDateTime>
#include <QVector>

#pragma once

namespace MinerGate {

class HRCounter {
public:
  enum {DEFAULT_HASHRATE_SIZE = 200};
  HRCounter(quint32 _size = DEFAULT_HASHRATE_SIZE);
  void addHRItem(quint64 _hashes);
  qreal getHashrate(quint32 _secs) const;
  void reset();
private:
  struct HRItem {
    HRItem(quint64 _hashes, const QDateTime &_time);
    HRItem();
    quint64 hashes;
    QDateTime time;
  };

  QVector<HRItem> m_hr_list;
  quint32 m_size;
  QVector<HRItem>::iterator m_latest;
  QVector<HRItem>::iterator m_oldest;
};

}
