#ifndef JELLYFINMANAGER_H
#define JELLYFINMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QWebSocket>
#include <QString>
#include <QList>
#include <QHash>
#include <QUrl>
#include <QMetaType>
#include <QTimer>
#include <QJsonObject>

class NounoursEngine;

struct JellyfinItem
{
    QString id, name, type, overview, seriesName;
    QString libraryName; // name of the Jellyfin library (media folder) this item belongs to
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

    // reports the current playback state to the Jellyfin server so it shows up
    // in the server's "Now Playing" activity. playState matches Mpv::PlayState.
    void UpdatePlaybackState(const QString &url, int playState);

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

    // Jellyfin "now playing" session reporting
    QString activePlaybackItemId, playSessionId;
    bool activePlaybackPaused = false;
    QTimer *playbackProgressTimer;

    // Jellyfin remote-control: WebSocket session used to receive
    // play/pause/stop/seek/volume/message commands sent from the server
    QWebSocket *socket;

    QString AuthHeader() const;
    static QNetworkRequest BuildRequest(const QUrl &url);
    void FetchServerName();
    void SearchAllLibraries(const QString &term, const QHash<QString, QString> &libraries);
    static QString ItemIdFromUrl(const QString &url);
    void PostSessionEvent(const QString &endpoint, QJsonObject body);
    void ReportPlaybackStart(const QString &itemId);
    void ReportPlaybackProgress();
    void ReportPlaybackStopped();

    void ReportCapabilities();
    void ConnectWebSocket();
    void HandleSocketMessage(const QString &message);
    void HandleGeneralCommand(const QJsonObject &data);
    void HandlePlaystateCommand(const QJsonObject &data);
    void HandlePlayCommand(const QJsonObject &data);
};

#endif // JELLYFINMANAGER_H
