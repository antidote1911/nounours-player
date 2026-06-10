#include "jellyfinmanager.h"

#include "nounoursengine.h"
#include "mpvhandler.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QUuid>

JellyfinManager::JellyfinManager(QObject *parent):
    QObject(parent),
    nounours(static_cast<NounoursEngine*>(parent)),
    manager(new QNetworkAccessManager(this)),
    playbackProgressTimer(new QTimer(this))
{
    // periodically report the playback position while a Jellyfin item is playing
    playbackProgressTimer->setInterval(10000);
    connect(playbackProgressTimer, &QTimer::timeout, this, &JellyfinManager::ReportPlaybackProgress);

    // continue playback into the next season once its episodes have been fetched
    connect(this, &JellyfinManager::episodesResultsSignal, this, [=](QString seasonId, QList<JellyfinItem> items)
    {
        if(pendingNextSeasonIndex < 0 || seasonId != nowPlayingSeasons[pendingNextSeasonIndex].id)
            return;

        nowPlayingSeasonIndex = pendingNextSeasonIndex;
        pendingNextSeasonIndex = -1;
        nowPlayingEpisodes = items;

        QStringList urls, labels;
        for(const auto &episode : items)
        {
            urls << GetStreamUrl(episode.id);
            labels << QString("%0. %1").arg(episode.index).arg(episode.name);
        }

        nounours->mpv->LoadUrlPlaylist(urls, labels, 0);
    });
}

JellyfinManager::~JellyfinManager()
{
}

QString JellyfinManager::AuthHeader() const
{
    return QString("MediaBrowser Client=\"Nounours Player\", Device=\"NounoursPlayer (%1)\", DeviceId=\"%0\", Version=\"%1\"")
            .arg(deviceId, NOUNOURS_PLAYER_VERSION);
}

QNetworkRequest JellyfinManager::BuildRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    // identify as the official-ish Nounours Player to reverse proxies that
    // block requests with a generic/script User-Agent (e.g. Qt's default)
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NounoursPlayer/" NOUNOURS_PLAYER_VERSION));
    return request;
}

void JellyfinManager::Connect(const QString &url, const QString &user, const QString &pass)
{
    serverUrl = url;
    username = user;
    password = pass;
    Connect();
}

void JellyfinManager::Connect()
{
    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QNetworkRequest request = BuildRequest(QUrl(url + "/Users/AuthenticateByName"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Emby-Authorization", AuthHeader().toUtf8());

    QJsonObject body;
    body["Username"] = username;
    body["Pw"] = password;

    accessToken.clear();
    userId.clear();

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            emit connectionFailedSignal(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        accessToken = response["AccessToken"].toString();
        userId = response["User"].toObject()["Id"].toString();

        if(accessToken.isEmpty() || userId.isEmpty())
            emit connectionFailedSignal(tr("Invalid response from server."));
        else
        {
            FetchServerName();
            emit connectedSignal();
        }

        reply->deleteLater();
    });
}

void JellyfinManager::FetchServerName()
{
    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QNetworkReply *reply = manager->get(BuildRequest(QUrl(url + "/System/Info/Public")));
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() == QNetworkReply::NoError)
            serverName = QJsonDocument::fromJson(reply->readAll()).object()["ServerName"].toString();
        reply->deleteLater();
    });
}

void JellyfinManager::Search(const QString &term)
{
    if(accessToken.isEmpty())
    {
        emit connectionFailedSignal(tr("Not connected."));
        return;
    }

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    // first list the user's libraries (media folders) so each result can be
    // tagged with the library it belongs to (Movies, Series, Animes, ...)
    QUrl viewsUrl(url + "/Users/" + userId + "/Views");
    QNetworkRequest viewsRequest = BuildRequest(viewsUrl);
    viewsRequest.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    QNetworkReply *viewsReply = manager->get(viewsRequest);
    connect(viewsReply, &QNetworkReply::finished, this, [=]
    {
        QHash<QString, QString> libraries;
        if(viewsReply->error() == QNetworkReply::NoError)
        {
            static const QSet<QString> excludedTypes = {"music", "musicvideos", "books", "photos", "livetv", "playlists", "boxsets"};

            QJsonObject response = QJsonDocument::fromJson(viewsReply->readAll()).object();
            for(auto entry : response["Items"].toArray())
            {
                QJsonObject obj = entry.toObject();
                if(excludedTypes.contains(obj["CollectionType"].toString()))
                    continue;

                libraries[obj["Id"].toString()] = obj["Name"].toString();
            }
        }
        viewsReply->deleteLater();

        SearchAllLibraries(term, libraries);
    });
}

void JellyfinManager::SearchAllLibraries(const QString &term, const QHash<QString, QString> &libraries)
{
    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    auto buildRequest = [=](const QString &parentId, bool withFields)
    {
        QUrlQuery query;
        query.addQueryItem("searchTerm", term);
        // "Video" covers items from "Home Videos" libraries (e.g. "Full Bluray"),
        // which Jellyfin does not classify as "Movie"
        query.addQueryItem("IncludeItemTypes", "Movie,Series,Video");
        query.addQueryItem("Recursive", "true");
        query.addQueryItem("Limit", "50");
        if(withFields)
            query.addQueryItem("Fields", "Overview,ProductionYear");
        if(!parentId.isEmpty())
            query.addQueryItem("ParentId", parentId);

        QUrl requestUrl(url + "/Users/" + userId + "/Items");
        requestUrl.setQuery(query);

        QNetworkRequest request = BuildRequest(requestUrl);
        request.setRawHeader("X-Emby-Token", accessToken.toUtf8());
        return request;
    };

    auto allItems = QSharedPointer<QList<JellyfinItem>>::create();
    auto libraryByItem = QSharedPointer<QHash<QString, QString>>::create();
    auto pending = QSharedPointer<int>::create(1 + libraries.size());

    auto finish = [=]
    {
        if(--(*pending) != 0)
            return;

        QList<JellyfinItem> items = *allItems;
        for(auto &item : items)
            item.libraryName = libraryByItem->value(item.id);

        emit searchResultsSignal(items);
    };

    // global, unscoped search: source of truth for which items match,
    // regardless of whether their library is exposed via /Users/{userId}/Views
    QNetworkReply *globalReply = manager->get(buildRequest(QString(), true));
    connect(globalReply, &QNetworkReply::finished, this, [=]
    {
        if(globalReply->error() == QNetworkReply::NoError)
        {
            QJsonObject response = QJsonDocument::fromJson(globalReply->readAll()).object();
            for(auto entry : response["Items"].toArray())
            {
                QJsonObject obj = entry.toObject();
                JellyfinItem item;
                item.id = obj["Id"].toString();
                item.name = obj["Name"].toString();
                item.type = obj["Type"].toString();
                item.overview = obj["Overview"].toString();
                item.year = obj["ProductionYear"].toInt();
                allItems->append(item);
            }
        }
        else
            emit connectionFailedSignal(globalReply->errorString());

        globalReply->deleteLater();
        finish();
    });

    // per-library searches: best-effort lookup used only to label results
    // with the name of the library they belong to
    for(auto it = libraries.constBegin(); it != libraries.constEnd(); ++it)
    {
        QString libraryName = it.value();
        QNetworkReply *reply = manager->get(buildRequest(it.key(), false));
        connect(reply, &QNetworkReply::finished, this, [=]
        {
            if(reply->error() == QNetworkReply::NoError)
            {
                QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
                for(auto entry : response["Items"].toArray())
                    (*libraryByItem)[entry.toObject()["Id"].toString()] = libraryName;
            }
            reply->deleteLater();
            finish();
        });
    }
}

void JellyfinManager::GetSeasons(const QString &seriesId)
{
    if(accessToken.isEmpty())
    {
        emit connectionFailedSignal(tr("Not connected."));
        return;
    }

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QUrlQuery query;
    query.addQueryItem("userId", userId);
    query.addQueryItem("Fields", "Overview");

    QUrl requestUrl(url + "/Shows/" + seriesId + "/Seasons");
    requestUrl.setQuery(query);

    QNetworkRequest request = BuildRequest(requestUrl);
    request.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            emit connectionFailedSignal(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        QList<JellyfinItem> items;
        for(auto entry : response["Items"].toArray())
        {
            QJsonObject obj = entry.toObject();
            JellyfinItem item;
            item.id = obj["Id"].toString();
            item.name = obj["Name"].toString();
            item.type = obj["Type"].toString();
            item.overview = obj["Overview"].toString();
            item.index = obj["IndexNumber"].toInt();
            items.append(item);
        }

        emit seasonsResultsSignal(seriesId, items);
        reply->deleteLater();
    });
}

void JellyfinManager::GetEpisodes(const QString &seriesId, const QString &seasonId)
{
    if(accessToken.isEmpty())
    {
        emit connectionFailedSignal(tr("Not connected."));
        return;
    }

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QUrlQuery query;
    query.addQueryItem("seasonId", seasonId);
    query.addQueryItem("userId", userId);
    query.addQueryItem("Fields", "Overview");

    QUrl requestUrl(url + "/Shows/" + seriesId + "/Episodes");
    requestUrl.setQuery(query);

    QNetworkRequest request = BuildRequest(requestUrl);
    request.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            emit connectionFailedSignal(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        QList<JellyfinItem> items;
        for(auto entry : response["Items"].toArray())
        {
            QJsonObject obj = entry.toObject();
            JellyfinItem item;
            item.id = obj["Id"].toString();
            item.name = obj["Name"].toString();
            item.type = obj["Type"].toString();
            item.overview = obj["Overview"].toString();
            item.seriesName = obj["SeriesName"].toString();
            item.parentIndex = obj["ParentIndexNumber"].toInt();
            item.index = obj["IndexNumber"].toInt();
            items.append(item);
        }

        emit episodesResultsSignal(seasonId, items);
        reply->deleteLater();
    });
}

void JellyfinManager::GetGenres()
{
    if(accessToken.isEmpty())
    {
        emit connectionFailedSignal(tr("Not connected."));
        return;
    }

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QUrlQuery query;
    query.addQueryItem("userId", userId);
    query.addQueryItem("IncludeItemTypes", "Movie,Series");
    query.addQueryItem("Recursive", "true");
    query.addQueryItem("SortBy", "SortName");

    QUrl requestUrl(url + "/Genres");
    requestUrl.setQuery(query);

    QNetworkRequest request = BuildRequest(requestUrl);
    request.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            emit connectionFailedSignal(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        QList<JellyfinItem> items;
        for(auto entry : response["Items"].toArray())
        {
            QJsonObject obj = entry.toObject();
            JellyfinItem item;
            item.id = obj["Id"].toString();
            item.name = obj["Name"].toString();
            item.type = obj["Type"].toString();
            items.append(item);
        }

        emit genresResultsSignal(items);
        reply->deleteLater();
    });
}

void JellyfinManager::GetItemsByGenre(const QString &genreId)
{
    if(accessToken.isEmpty())
    {
        emit connectionFailedSignal(tr("Not connected."));
        return;
    }

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QUrlQuery query;
    query.addQueryItem("GenreIds", genreId);
    query.addQueryItem("IncludeItemTypes", "Movie,Series");
    query.addQueryItem("Recursive", "true");
    query.addQueryItem("Fields", "Overview,ProductionYear");
    query.addQueryItem("SortBy", "SortName");

    QUrl requestUrl(url + "/Users/" + userId + "/Items");
    requestUrl.setQuery(query);

    QNetworkRequest request = BuildRequest(requestUrl);
    request.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            emit connectionFailedSignal(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        QList<JellyfinItem> items;
        for(auto entry : response["Items"].toArray())
        {
            QJsonObject obj = entry.toObject();
            JellyfinItem item;
            item.id = obj["Id"].toString();
            item.name = obj["Name"].toString();
            item.type = obj["Type"].toString();
            item.overview = obj["Overview"].toString();
            item.year = obj["ProductionYear"].toInt();
            items.append(item);
        }

        emit genreItemsResultsSignal(genreId, items);
        reply->deleteLater();
    });
}

QString JellyfinManager::getNowPlayingTitle(const QString &currentUrl) const
{
    if(currentUrl == nowPlayingUrl)
        return nowPlayingTitle;

    for(const auto &episode : nowPlayingEpisodes)
    {
        if(GetStreamUrl(episode.id) == currentUrl)
        {
            return QString("%0: %1 - S%2E%3 - %4")
                    .arg(getServerName(), episode.seriesName)
                    .arg(episode.parentIndex, 2, 10, QChar('0'))
                    .arg(episode.index, 2, 10, QChar('0'))
                    .arg(episode.name);
        }
    }

    return QString();
}

void JellyfinManager::PlayMovie(const JellyfinItem &item)
{
    QString url = GetStreamUrl(item.id);
    QString label = item.name;
    if(item.year > 0)
        label += QString(" (%0)").arg(item.year);

    SetNowPlaying(url, QString("%0: %1").arg(getServerName(), label));
    nowPlayingEpisodes.clear();

    nounours->mpv->LoadUrlPlaylist({url}, {label}, 0);
}

void JellyfinManager::PlayEpisodes(const QString &seriesId, const QList<JellyfinItem> &seasons, int seasonIndex,
                                    const QList<JellyfinItem> &episodes, int episodeIndex)
{
    nowPlayingSeriesId = seriesId;
    nowPlayingSeasons = seasons;
    nowPlayingSeasonIndex = seasonIndex;
    nowPlayingEpisodes = episodes;
    pendingNextSeasonIndex = -1;

    QStringList urls, labels;
    for(const auto &episode : episodes)
    {
        urls << GetStreamUrl(episode.id);
        labels << QString("%0. %1").arg(episode.index).arg(episode.name);
    }

    nounours->mpv->LoadUrlPlaylist(urls, labels, episodeIndex);
}

bool JellyfinManager::PlayNextSeason()
{
    if(nowPlayingSeasonIndex < 0 || nowPlayingSeasonIndex + 1 >= nowPlayingSeasons.size())
        return false;

    pendingNextSeasonIndex = nowPlayingSeasonIndex + 1;
    GetEpisodes(nowPlayingSeriesId, nowPlayingSeasons[pendingNextSeasonIndex].id);
    return true;
}

QString JellyfinManager::ItemIdFromUrl(const QString &url)
{
    static const QRegularExpression re("/Videos/([^/]+)/stream");
    QRegularExpressionMatch match = re.match(url);
    return match.hasMatch() ? match.captured(1) : QString();
}

void JellyfinManager::PostSessionEvent(const QString &endpoint, QJsonObject body)
{
    if(accessToken.isEmpty())
        return;

    QString url = serverUrl;
    if(url.endsWith('/'))
        url.chop(1);

    QNetworkRequest request = BuildRequest(QUrl(url + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Emby-Token", accessToken.toUtf8());

    body["PlaySessionId"] = playSessionId;
    body["IsPaused"] = activePlaybackPaused;
    body["IsMuted"] = nounours->mpv->getMute();
    body["VolumeLevel"] = nounours->mpv->getVolume();
    body["PositionTicks"] = qint64(nounours->mpv->getTime()) * 10000000LL;
    body["CanSeek"] = true;
    body["PlayMethod"] = "DirectStream";

    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void JellyfinManager::ReportPlaybackStart(const QString &itemId)
{
    PostSessionEvent("/Sessions/Playing", {{"ItemId", itemId}});
}

void JellyfinManager::ReportPlaybackProgress()
{
    if(activePlaybackItemId.isEmpty())
        return;

    PostSessionEvent("/Sessions/Playing/Progress", {{"ItemId", activePlaybackItemId}});
}

void JellyfinManager::ReportPlaybackStopped()
{
    PostSessionEvent("/Sessions/Playing/Stopped", {{"ItemId", activePlaybackItemId}});
}

void JellyfinManager::UpdatePlaybackState(const QString &url, int playState)
{
    QString itemId = ItemIdFromUrl(url);

    if(itemId != activePlaybackItemId)
    {
        if(!activePlaybackItemId.isEmpty())
            ReportPlaybackStopped();

        activePlaybackItemId = itemId;
        playbackProgressTimer->stop();

        if(activePlaybackItemId.isEmpty())
            return;

        playSessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        activePlaybackPaused = (playState == Mpv::Paused);
        ReportPlaybackStart(activePlaybackItemId);
        playbackProgressTimer->start();
        return;
    }

    if(activePlaybackItemId.isEmpty())
        return;

    if(playState == Mpv::Stopped || playState == Mpv::Idle)
    {
        ReportPlaybackStopped();
        activePlaybackItemId.clear();
        playbackProgressTimer->stop();
        return;
    }

    bool paused = (playState == Mpv::Paused);
    if(paused != activePlaybackPaused)
    {
        activePlaybackPaused = paused;
        ReportPlaybackProgress();
    }
}
