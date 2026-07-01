#include "subtitledownloader.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QUrl>

#include "bittorrent/torrent.h"
#include "logger.h"
#include "path.h"

using namespace Qt::StringLiterals;

SubtitleDownloader::SubtitleDownloader(QObject *parent)
    : QObject(parent)
{
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &SubtitleDownloader::handleReplyFinished);
}

void SubtitleDownloader::downloadSubtitlesForTorrent(const BitTorrent::Torrent *torrent)
{
    const QStringList videoExtensions = {
        u".mp4"_s, u".mkv"_s, u".avi"_s, u".mov"_s, u".wmv"_s, u".flv"_s, u".webm"_s, u".mpg"_s, u".mpeg"_s
    };

    m_activeRequests = 0;

    for (const Path &torrentRelpath : torrent->filePaths())
    {
        bool isVideo = false;
        for (const QString &ext : videoExtensions)
        {
            if (torrentRelpath.hasExtension(ext))
            {
                isVideo = true;
                break;
            }
        }

        if (isVideo)
        {
            Path absolutePath = torrent->savePath() / torrentRelpath;
            QString filePath = absolutePath.toString();
            QString hash = calculateSubDbHash(filePath);
            if (!hash.isEmpty())
            {
                startDownload(filePath, hash);
            }
        }
    }

    if (m_activeRequests == 0)
    {
        deleteLater();
    }
}

QString SubtitleDownloader::calculateSubDbHash(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    qint64 fileSize = file.size();
    if (fileSize < 65536)
        return {};

    // Read first 64KB
    QByteArray firstChunk = file.read(65536);
    if (firstChunk.size() < 65536)
        return {};

    // Read last 64KB
    if (!file.seek(fileSize - 65536))
        return {};

    QByteArray lastChunk = file.read(65536);
    if (lastChunk.size() < 65536)
        return {};

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(firstChunk);
    hash.addData(lastChunk);

    return QString::fromUtf8(hash.result().toHex().toLower());
}

void SubtitleDownloader::startDownload(const QString &filePath, const QString &hash)
{
    QUrl url(u"http://api.thesubdb.com/?action=download&hash=%1&language=en"_s.arg(hash));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArray("SubDB/1.0 (Vortex/1.0.0; https://github.com/vortex-torrent/Vortex)"));

    QNetworkReply *reply = m_networkManager.get(request);
    m_replyToFilePath[reply] = filePath;
    m_activeRequests++;

    LogMsg(tr("[Subtitle Downloader] Searching subtitles for: %1 (Hash: %2)")
           .arg(QFileInfo(filePath).fileName(), hash), Log::NORMAL);
}

void SubtitleDownloader::handleReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    m_activeRequests--;

    QString filePath = m_replyToFilePath.take(reply);
    if (filePath.isEmpty())
    {
        if (m_activeRequests == 0)
            deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError)
    {
        QFileInfo videoInfo(filePath);
        QString srtPath = videoInfo.absolutePath() + u"/"_s + videoInfo.completeBaseName() + u".srt"_s;

        QFile srtFile(srtPath);
        if (srtFile.open(QIODevice::WriteOnly))
        {
            srtFile.write(reply->readAll());
            srtFile.close();
            LogMsg(tr("[Subtitle Downloader] Subtitle downloaded successfully for: %1")
                   .arg(videoInfo.fileName()), Log::NORMAL);
        }
        else
        {
            LogMsg(tr("[Subtitle Downloader] Failed to write srt file for: %1")
                   .arg(videoInfo.fileName()), Log::WARNING);
        }
    }
    else
    {
        QFileInfo videoInfo(filePath);
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            LogMsg(tr("[Subtitle Downloader] Subtitle not found on SubDb for: %1")
                   .arg(videoInfo.fileName()), Log::INFO);
        }
        else
        {
            LogMsg(tr("[Subtitle Downloader] Download failed for: %1. Error: %2")
                   .arg(videoInfo.fileName(), reply->errorString()), Log::WARNING);
        }
    }

    if (m_activeRequests == 0)
    {
        deleteLater();
    }
}
