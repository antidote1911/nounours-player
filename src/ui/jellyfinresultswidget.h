#ifndef JELLYFINRESULTSWIDGET_H
#define JELLYFINRESULTSWIDGET_H

#include <QWidget>
#include <QList>
#include <QHash>

#include "../jellyfinmanager.h"

class NounoursEngine;

class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QPushButton;

class JellyfinResultsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JellyfinResultsWidget(NounoursEngine *nounours, QWidget *parent = nullptr);

    void SetResults(const QList<JellyfinItem> &items);

private:
    NounoursEngine  *nounours;
    JellyfinManager *jellyfin;

    QTreeWidget *resultsList;
    QTextEdit   *synopsisEdit;
    QPushButton *playButton;

    QHash<QString, QTreeWidgetItem*> seriesNodes;
    QHash<QString, QTreeWidgetItem*> seasonNodes;

    void OnItemExpanded(QTreeWidgetItem *item);
    void DoPlay();
};

#endif // JELLYFINRESULTSWIDGET_H
