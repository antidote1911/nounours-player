#include "jellyfinmanager.h"

#include "nounoursengine.h"
#include "mpvhandler.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

JellyfinManager::JellyfinManager(QObject *parent):
    QObject(parent),
    nounours(static_cast<NounoursEngine*>(parent)),
    manager(new QNetworkAccessManager(this))
{
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
    return QString("MediaBrowser Client=\"Nounours Player\", Device=\"Desktop\", DeviceId=\"%0\", Version=\"%1\"")
            .arg(deviceId, NOUNOURS_PLAYER_VERSION);
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

    QNetworkRequest request(QUrl(url + "/Users/AuthenticateByName"));
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

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url + "/System/Info/Public")));
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

    QUrlQuery query;
    query.addQueryItem("searchTerm", term);
    query.addQueryItem("IncludeItemTypes", "Movie,Series");
    query.addQueryItem("Recursive", "true");
    query.addQueryItem("Fields", "Overview,ProductionYear");
    query.addQueryItem("Limit", "50");

    QUrl requestUrl(url + "/Users/" + userId + "/Items");
    requestUrl.setQuery(query);

    QNetworkRequest request(requestUrl);
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

        emit searchResultsSignal(items);
        reply->deleteLater();
    });
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

    QNetworkRequest request(requestUrl);
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

    QNetworkRequest request(requestUrl);
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

    QNetworkRequest request(requestUrl);
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

    QNetworkRequest request(requestUrl);
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
