#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QListWidget>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>

class NounoursEngine;

class NumberedDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        int num = index.data(Qt::UserRole).toInt();
        if(num == 0) num = index.row() + 1;
        opt.text = QString::number(num) + ".  " + opt.text;
        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    }
};

class PlaylistWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget *parent = 0);

    void AttachEngine(NounoursEngine *nounours);

public slots:
    void Populate();
    void RefreshPlaylist();

    void SelectItem(const QString &item);
    QString CurrentItem();
    int CurrentIndex(); // index of the current playing file
    void SelectIndex(int index, bool relative = false); // relative to current selection
    void PlayIndex(int index, bool relative = false); // relative to current playing file
    void RemoveIndex(int index); // remove the selected item

    void Search(const QString&);
    void ShowAll(bool);
    void Shuffle();
    void SavePlaylist();

protected slots:
    void BoldText(const QString &f, bool state);
    void RemoveFromPlaylist(QListWidgetItem *item);
    void DeleteFromDisk(QListWidgetItem *item);

private:
    void Renumber();
    QString ItemFile(QListWidgetItem *item) const;

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void dropEvent(QDropEvent *event);

private:
    NounoursEngine *nounours;

    QStringList playlist, labels;
    QString file, suffix;
    bool newPlaylist,
         refresh,
         showAll,
         shuffled = false;
};

#endif // PLAYLISTWIDGET_H
