#include "jellyfinresultswidget.h"

#include "../nounoursengine.h"
#include "../mpvhandler.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QSplitter>

JellyfinResultsWidget::JellyfinResultsWidget(NounoursEngine *nounours, QWidget *parent)
    : QWidget(parent), nounours(nounours), jellyfin(nounours->jellyfin)
{
    resultsList = new QTreeWidget(this);
    resultsList->setHeaderHidden(true);
    resultsList->setColumnCount(1);
    synopsisEdit = new QTextEdit(this);
    synopsisEdit->setReadOnly(true);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(resultsList);
    splitter->addWidget(synopsisEdit);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    playButton = new QPushButton(tr("Play"), this);
    playButton->setEnabled(false);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(playButton);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(splitter);
    root->addLayout(buttonLayout);

    connect(jellyfin, &JellyfinManager::seasonsResultsSignal, this, [=](QString seriesId, QList<JellyfinItem> items)
    {
        QTreeWidgetItem *seriesItem = seriesNodes.value(seriesId);
        if(!seriesItem)
            return;

        qDeleteAll(seriesItem->takeChildren());

        for(const auto &season : items)
        {
            auto *node = new QTreeWidgetItem(seriesItem);
            node->setText(0, season.name.isEmpty() ? tr("Season %0").arg(season.index) : season.name);
            node->setData(0, Qt::UserRole, QVariant::fromValue(season));

            auto *placeholder = new QTreeWidgetItem(node);
            placeholder->setText(0, tr("Loading..."));
            seasonNodes[season.id] = node;
        }
    });

    connect(jellyfin, &JellyfinManager::episodesResultsSignal, this, [=](QString seasonId, QList<JellyfinItem> items)
    {
        QTreeWidgetItem *seasonItem = seasonNodes.value(seasonId);
        if(!seasonItem)
            return;

        qDeleteAll(seasonItem->takeChildren());

        for(const auto &episode : items)
        {
            auto *node = new QTreeWidgetItem(seasonItem);
            node->setText(0, QString("%0. %1").arg(episode.index).arg(episode.name));
            node->setData(0, Qt::UserRole, QVariant::fromValue(episode));
        }
    });

    connect(resultsList, &QTreeWidget::itemExpanded, this, &JellyfinResultsWidget::OnItemExpanded);

    connect(resultsList, &QTreeWidget::currentItemChanged, this, [=](QTreeWidgetItem *current, QTreeWidgetItem *)
    {
        QVariant data = current ? current->data(0, Qt::UserRole) : QVariant();
        if(data.isNull())
        {
            playButton->setEnabled(false);
            synopsisEdit->clear();
            return;
        }
        JellyfinItem item = data.value<JellyfinItem>();
        synopsisEdit->setPlainText(item.overview.isEmpty() ? tr("No synopsis available.") : item.overview);
        playButton->setEnabled(item.type == "Movie" || item.type == "Episode");
    });

    connect(playButton, &QPushButton::clicked, this, &JellyfinResultsWidget::DoPlay);
    connect(resultsList, &QTreeWidget::itemDoubleClicked, this, &JellyfinResultsWidget::DoPlay);
}

void JellyfinResultsWidget::SetResults(const QList<JellyfinItem> &items)
{
    resultsList->clear();
    synopsisEdit->clear();
    seriesNodes.clear();
    seasonNodes.clear();

    for(const auto &item : items)
    {
        QString label = QString("[%0] %1").arg(item.type == "Movie" ? tr("Movie") : tr("Series"), item.name);
        if(item.year > 0)
            label += QString(" (%0)").arg(item.year);

        auto *node = new QTreeWidgetItem(resultsList);
        node->setText(0, label);
        node->setData(0, Qt::UserRole, QVariant::fromValue(item));

        if(item.type == "Series")
        {
            auto *placeholder = new QTreeWidgetItem(node);
            placeholder->setText(0, tr("Loading..."));
            seriesNodes[item.id] = node;
        }
    }
}

void JellyfinResultsWidget::OnItemExpanded(QTreeWidgetItem *item)
{
    // not yet loaded if the only child is the "Loading..." placeholder (no item data)
    if(item->childCount() != 1 || !item->child(0)->data(0, Qt::UserRole).isNull())
        return;

    JellyfinItem data = item->data(0, Qt::UserRole).value<JellyfinItem>();
    if(data.type == "Series")
        jellyfin->GetSeasons(data.id);
    else if(data.type == "Season")
    {
        JellyfinItem seriesData = item->parent()->data(0, Qt::UserRole).value<JellyfinItem>();
        jellyfin->GetEpisodes(seriesData.id, data.id);
    }
}

void JellyfinResultsWidget::DoPlay()
{
    QTreeWidgetItem *current = resultsList->currentItem();
    if(!current)
        return;

    QVariant data = current->data(0, Qt::UserRole);
    if(data.isNull())
        return;

    JellyfinItem item = data.value<JellyfinItem>();
    if(item.type != "Movie" && item.type != "Episode")
        return;

    if(item.type == "Episode")
    {
        QTreeWidgetItem *seasonItem = current->parent();
        QTreeWidgetItem *seriesItem = seasonItem ? seasonItem->parent() : nullptr;
        if(!seasonItem || !seriesItem)
            return;

        QList<JellyfinItem> episodes;
        int episodeIndex = -1;
        for(int i = 0; i < seasonItem->childCount(); i++)
        {
            QVariant d = seasonItem->child(i)->data(0, Qt::UserRole);
            if(d.isNull())
                continue;
            JellyfinItem episode = d.value<JellyfinItem>();
            if(episode.id == item.id)
                episodeIndex = episodes.size();
            episodes.append(episode);
        }

        QList<JellyfinItem> seasons;
        int seasonIndex = -1;
        JellyfinItem seasonData = seasonItem->data(0, Qt::UserRole).value<JellyfinItem>();
        for(int i = 0; i < seriesItem->childCount(); i++)
        {
            QVariant d = seriesItem->child(i)->data(0, Qt::UserRole);
            if(d.isNull())
                continue;
            JellyfinItem season = d.value<JellyfinItem>();
            if(season.id == seasonData.id)
                seasonIndex = seasons.size();
            seasons.append(season);
        }

        JellyfinItem seriesData = seriesItem->data(0, Qt::UserRole).value<JellyfinItem>();
        jellyfin->PlayEpisodes(seriesData.id, seasons, seasonIndex, episodes, episodeIndex);
    }
    else
        jellyfin->PlayMovie(item);

    window()->close();
}
