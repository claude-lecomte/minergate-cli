#pragma once

#include <QSharedPointer>
#include <QNetworkRequest>

class QNetworkReply;

namespace mining
{
    struct job_details_native
    {
        std::vector<quint8> blob;
    quint32 target;
        std::string job_id;
    quint32 time_to_live;
    };
}

namespace MinerGate
{
  class DownloadManager;

  typedef QSharedPointer<QNetworkReply> NetworkReplyPtr;
  typedef QSharedPointer<DownloadManager> DownloadManagerPtr;
}
