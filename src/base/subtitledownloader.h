#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace BitTorrent
{
    class Torrent;
}

class SubtitleDownloader final : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleDownloader(QObject *parent = nullptr);
    ~SubtitleDownloader() override = default;

    void downloadSubtitlesForTorrent(const BitTorrent::Torrent *torrent);

private slots:
    void handleReplyFinished(QNetworkReply *reply);

private:
    QString calculateSubDbHash(const QString &filePath);
    void startDownload(const QString &filePath, const QString &hash);

    QNetworkAccessManager m_networkManager;
    QHash<QNetworkReply*, QString> m_replyToFilePath;
    int m_activeRequests = 0;
};
