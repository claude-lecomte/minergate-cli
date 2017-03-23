#include "miner-abstract/hrcounter.h"

namespace MinerGate {

HRCounter::HRCounter(quint32 _size) :
  m_hr_list(_size)
, m_size(_size)
, m_latest()
, m_oldest()
{
  reset();
}

HRCounter::HRItem::HRItem(quint64 _hashes, const QDateTime &_time) :
  hashes(_hashes)
, time(_time) {}

HRCounter::HRItem::HRItem() :
  hashes(0)
, time() {}

void HRCounter::addHRItem(quint64 _hashes) {
  HRItem new_item(_hashes, QDateTime::currentDateTime());
  *m_latest = new_item;

  if (++m_latest == m_hr_list.end()) {
    m_latest = m_hr_list.begin();
  }

  if (m_latest == m_oldest && ++m_oldest == m_hr_list.end()) {
    m_oldest = m_hr_list.begin();
  }
}

qreal HRCounter::getHashrate(quint32 _secs) const {
  if (Q_UNLIKELY(m_oldest->time.isNull())) {
    return 0;
  }

  QVector<HRItem>::const_iterator last = m_latest;
  if (last == m_hr_list.constEnd()) {
    return 0;
  }

  if (last == m_hr_list.begin()) {
    last = m_hr_list.constEnd();
  }

  --last;
  if (!_secs || m_oldest->time.secsTo(last->time) < _secs) {
    qreal time_diff = m_oldest->time.msecsTo(last->time);
    size_t hashes = 0;
    for (size_t i = 0; i < m_size; ++i) {
      hashes += m_hr_list[i].hashes;
    }

    if (time_diff == 0) {
      return 0;
    }

    return hashes / (time_diff / 1000.);
  }

  QVector<HRItem>::const_iterator tmp = last;
  qreal time_diff = 0;
  quint64 hashes = 0;
  do {
    hashes += tmp->hashes;
    if(tmp == m_hr_list.constBegin()) {
      tmp = m_hr_list.constEnd();
    }

    --tmp;
    time_diff = tmp ->time.msecsTo(last->time);
  } while(tmp->time.msecsTo(last->time) / 1000. < _secs && tmp != m_oldest);
  if (time_diff == 0) {
    return 0;
  }

  return hashes / (time_diff / 1000.);
}

void HRCounter::reset()
{
  m_hr_list.clear();
  m_hr_list.fill(HRItem(0, QDateTime()), m_size);
  m_latest = m_hr_list.begin();
  m_oldest = m_hr_list.begin();
}

}
