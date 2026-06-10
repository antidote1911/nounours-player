#ifndef JELLYFINMANAGER_H
#define JELLYFINMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QList>
#include <QUrl>
#include <QMetaType>

class NounoursEngine;

struct JellyfinItem
{
    QString id, name, type, overview, seriesName;
    int year = 0;
    int parentIndex = 0; // season number, for episodes
    int index = 0;       // season or episode number
};

Q_DECLARE_METATYPE(JellyfinItem)

class JellyfinManager : public QObject
{
friend class NounoursEngine;
    Q_OBJECT
public:
    explicit JellyfinManager(QObject *parent = 0);
    ~JellyfinManager();

    QString getServerUrl() { return serverUrl; }
    QString getUsername()  { return username; }
    QString getPassword()  { return password; }
    bool    isConnected()  { return !accessToken.isEmpty(); }

    QString GetStreamUrl(const QString &itemId) const
    {
        return QString("%0/Videos/%1/stream?static=true&api_key=%2").arg(serverUrl, itemId, accessToken);
    }

    QString getServerName() const { return serverName.isEmpty() ? QUrl(serverUrl).host() : serverName; }
    QString getNowPlayingTitle(const QString &currentUrl) const;

public slots:
    void ServerUrl(QString s) { serverUrl = s; }
    void Username(QString s)  { username = s; }
    void Password(QString s)  { password = s; }

    void Connect();
    void Connect(const QString &url, const QString &user, const QString &pass);
    void Search(const QString &term);
    void GetSeasons(const QString &seriesId);
    void GetEpisodes(const QString &seriesId, const QString &seasonId);
    void GetGenres();
    void GetItemsByGenre(const QString &genreId);
    void SetNowPlaying(QString url, QString title) { nowPlayingUrl = url; nowPlayingTitle = title; }

    void PlayMovie(const JellyfinItem &item);
    void PlayEpisodes(const QString &seriesId, const QList<JellyfinItem> &seasons, int seasonIndex,
                       const QList<JellyfinItem> &episodes, int episodeIndex);
    bool PlayNextSeason();

signals:
    void connectedSignal();
    void connectionFailedSignal(QString error);
    void searchResultsSignal(QList<JellyfinItem> items);
    void seasonsResultsSignal(QString seriesId, QList<JellyfinItem> items);
    void episodesResultsSignal(QString seasonId, QList<JellyfinItem> items);
    void genresResultsSignal(QList<JellyfinItem> items);
    void genreItemsResultsSignal(QString genreId, QList<JellyfinItem> items);
    void messageSignal(QString msg);

private:
    NounoursEngine *nounours;
    QNetworkAccessManager *manager;

    QString serverUrl, username, password;
    QString accessToken, userId, deviceId;
    QString serverName;
    QString nowPlayingUrl, nowPlayingTitle;
    QList<JellyfinItem> nowPlayingEpisodes;

    // currently playing episode/season context, used for season rollover
    QString nowPlayingSeriesId;
    QList<JellyfinItem> nowPlayingSeasons;
    int nowPlayingSeasonIndex = -1;
    int pendingNextSeasonIndex = -1;

    QString AuthHeader() const;
    void FetchServerName();
};

#endif // JELLYFINMANAGER_H
