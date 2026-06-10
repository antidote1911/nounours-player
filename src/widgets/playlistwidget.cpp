#include "playlistwidget.h"

#include "nounoursengine.h"
#include "mpvhandler.h"

#include <QFile>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QFont>
#include <QMessageBox>
#include <QTextStream>

#include <algorithm>
#include <random>

PlaylistWidget::PlaylistWidget(QWidget *parent) :
    QListWidget(parent),
    newPlaylist(false),
    refresh(false),
    showAll(true)
{
    setAttribute(Qt::WA_NoMousePropagation);
    setItemDelegate(new NumberedDelegate(this));
}

void PlaylistWidget::AttachEngine(NounoursEngine *nounours)
{
    this->nounours = nounours;
    connect(nounours->mpv, &MpvHandler::playlistChanged,
            [=](const QStringList &list, const QStringList &labelList)
            {
                playlist = list;
                labels = labelList;
                newPlaylist = true;
                if(refresh)
                {
                    Populate();
                    BoldText(file, true);
                    refresh = false;
                }
            });

    connect(nounours->mpv, &MpvHandler::fileChanged,
            [=](QString f)
            {
                if(newPlaylist)
                {
                    file = f;
                    if(f != QString())
                        suffix = file.split('.').last();
                    Populate();
                    BoldText(file, true);
                    newPlaylist = false;
                }
                else
                {
                    BoldText(file, false);
                    BoldText(f, true);
                    file = f;
                }
                SelectIndex(CurrentIndex());
            });

    connect(this, &PlaylistWidget::doubleClicked,
            [=](const QModelIndex &i)
            {
                PlayIndex(i.row());
            });
}

void PlaylistWidget::Populate()
{
    if(playlist.empty())
        return;

    QListWidgetItem *current = currentItem();
    QString item;
    if(current != nullptr)
        item = ItemFile(current);
    else
        item = file;

    shuffled = false;
    clear();
    for(int i = 0; i < playlist.size(); ++i)
    {
        if(showAll || playlist[i].endsWith(suffix))
        {
            QString text = (i < labels.size() && !labels[i].isEmpty()) ? labels[i] : playlist[i];
            QListWidgetItem *it = new QListWidgetItem(text);
            it->setData(Qt::UserRole, i + 1);
            it->setData(Qt::UserRole + 1, playlist[i]);
            addItem(it);
        }
    }
    BoldText(file, true);
    SelectItem(item);
}

void PlaylistWidget::RefreshPlaylist()
{
    refresh = true;
    nounours->mpv->LoadPlaylist(nounours->mpv->getPath()+file);
}

QString PlaylistWidget::CurrentItem()
{
    QListWidgetItem *current = currentItem();
    if(current != nullptr)
        return current->text();
    return QString();
}

int PlaylistWidget::CurrentIndex()
{
    for(int i = 0; i < count(); ++i)
    {
        if(ItemFile(item(i)) == file)
            return i;
    }
    return 0;
}

QString PlaylistWidget::ItemFile(QListWidgetItem *item) const
{
    QVariant v = item->data(Qt::UserRole + 1);
    return v.isValid() ? v.toString() : item->text();
}

void PlaylistWidget::SelectIndex(int index, bool relative)
{
    int newIndex = relative ? currentRow() : 0;
    if(newIndex < 0)
        newIndex = 0;

    newIndex += index;

    if(newIndex < 0)
        newIndex = 0;
    else if(newIndex > count())
        newIndex = count();

    setCurrentRow(newIndex);
    scrollToItem(currentItem());
}

void PlaylistWidget::PlayIndex(int index, bool relative)
{
    int newIndex = 0;
    if(relative)
        newIndex = CurrentIndex();
    newIndex += index;

    if(newIndex < 0)
        newIndex = 0;
    else if(newIndex > count())
        newIndex = count();

    QListWidgetItem *current = item(newIndex);
    if(current != nullptr)
    {
        scrollToItem(current);
        if(!nounours->mpv->PlayFile(ItemFile(current)))
        {
            PlayIndex(newIndex+1);
            RemoveIndex(newIndex);
        }
    }
}

void PlaylistWidget::RemoveIndex(int index)
{
    if(index < 0)
        index = 0;
    else if(index > count())
        index = count();

    QListWidgetItem *current = item(index);
    if(current != nullptr)
        RemoveFromPlaylist(current);
}

void PlaylistWidget::BoldText(const QString &f, bool state)
{
    for(int i = 0; i < count(); ++i)
    {
        QListWidgetItem *it = item(i);
        if(ItemFile(it) == f)
        {
            QFont font = it->font();
            font.setBold(state);
            it->setFont(font);
            return;
        }
    }
}

void PlaylistWidget::Search(const QString &s)
{
    QListWidgetItem *current = currentItem();
    QString item;
    if(current != nullptr)
        item = ItemFile(current);
    else
        item = file;

    QStringList newPlaylist;
    for(QStringList::iterator item = playlist.begin(); item != playlist.end(); item++)
    {
        if(item->contains(s, Qt::CaseInsensitive) && (showAll || item->endsWith(suffix)))
            newPlaylist.append(*item);
    }

    clear();
    addItems(newPlaylist);

    BoldText(file, true);
    SelectItem(item);
}

void PlaylistWidget::ShowAll(bool b)
{
    QListWidgetItem *current = currentItem();
    QString item;
    if(current != nullptr)
        item = ItemFile(current);
    else
        item = file;

    showAll = b;
    Populate();

    BoldText(file, true);
    SelectItem(item);
}

void PlaylistWidget::Shuffle()
{
    if(this->count() == 0)
        return;

    shuffled = true;
    QListWidgetItem *current = currentItem();
    QString item;
    if(current != nullptr)
        item = ItemFile(current);
    else
        item = file;

    // Take all items, preserving UserRole numbers
    QList<QListWidgetItem*> items;
    while(count() > 0)
        items.append(takeItem(0));

    std::shuffle(items.begin(), items.end(), std::mt19937{std::random_device{}()});

    // Make currently playing file first
    auto iter = std::find_if(items.begin(), items.end(),
                             [this](QListWidgetItem *it){ return ItemFile(it) == file; });
    if(iter != items.end())
        std::swap(*iter, *items.begin());

    for(auto *it : items)
        addItem(it);

    BoldText(file, true);
    SelectItem(item);
}

void PlaylistWidget::SelectItem(const QString &f)
{
    if(f != QString())
    {
        for(int i = 0; i < count(); ++i)
        {
            if(ItemFile(item(i)) == f)
            {
                setCurrentItem(item(i));
                scrollToItem(item(i));
                return;
            }
        }
    }
    setCurrentRow(0);
    scrollToItem(currentItem());
}

void PlaylistWidget::Renumber()
{
    if(shuffled)
        return;
    for(int i = 0; i < count(); ++i)
        item(i)->setData(Qt::UserRole, i + 1);
}

void PlaylistWidget::RemoveFromPlaylist(QListWidgetItem *item)
{
    playlist.removeOne(ItemFile(item));
    delete item;
    Renumber();
    emit currentRowChanged(currentRow());
}

void PlaylistWidget::DeleteFromDisk(QListWidgetItem *item)
{
    QString itemFile = ItemFile(item);

    if(QMessageBox::question(
            parentWidget(),
            tr("Delete from disk?"),
            tr("Are you sure you want to permanently delete \"%0\"?").arg(itemFile),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    playlist.removeOne(itemFile);
    QString r = itemFile.left(itemFile.lastIndexOf('.')+1); // get file root (no extension)
    // check and remove all subtitle_files with the same root as the video
    for(auto ext : Mpv::subtitle_filetypes)
    {
        QFile subf(r+ext.mid(2));
        if(subf.exists() &&
           QMessageBox::question(
            parentWidget(),
            tr("Delete sub-file?"),
            tr("Would you like to delete the associated sub file [%0]?").arg(
                subf.fileName())) == QMessageBox::Yes)
            subf.remove();
    }
    // check and remove all external subtitle files in the video
    for(auto track : nounours->mpv->getFileInfo().tracks)
    {
        if(track.external)
        {
            QFile subf(track.external_filename);
            if(subf.exists() &&
               QMessageBox::question(
                parentWidget(),
                tr("Delete external sub-file?"),
                tr("Would you like to delete the associated sub file [%0]?").arg(
                    subf.fileName())) == QMessageBox::Yes)
                subf.remove();
        }
    }
    // remove the actual file
    QFile f(nounours->mpv->getPath()+itemFile);
    f.remove();
    delete item;
    Renumber();
    emit currentRowChanged(currentRow());
}

void PlaylistWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());
    if(item != nullptr)
    {
        QMenu *menu = new QMenu();
        connect(menu->addAction(tr("R&emove from Playlist")), &QAction::triggered, // Playlist: Remove from playlist (right-click)
                [=]
                {
                    RemoveFromPlaylist(item);
                });
        connect(menu->addAction(tr("&Delete from Disk")), &QAction::triggered,     // Playlist: Delete from Disk (right-click)
                [=]
                {
                    DeleteFromDisk(item);
                });
        connect(menu->addAction(tr("&Refresh")), &QAction::triggered,              // Playlist: Refresh (right-click)
                [=]
                {
                    RefreshPlaylist();
                });
        menu->exec(viewport()->mapToGlobal(event->pos()));
        delete menu;
    }
}

void PlaylistWidget::SavePlaylist()
{
    if(playlist.isEmpty())
        return;

    QString basePath = nounours->mpv->getPath();
    QString suggested = QDir(basePath).dirName() + ".m3u";

    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Save Playlist"),
        QDir::homePath() + "/" + suggested,
        tr("M3U Playlist (*.m3u *.m3u8)")
    );

    if(filePath.isEmpty())
        return;

    QFile f(filePath);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&f);
    out << "#EXTM3U\n";
    for(const QString &item : playlist)
        out << basePath << item << "\n";
    f.close();

    if(QMessageBox::question(
            this,
            tr("Playlist saved"),
            tr("Playlist saved. Open it now?"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        nounours->mpv->LoadFile(filePath);
    }
}

void PlaylistWidget::dropEvent(QDropEvent *event)
{
    QListWidget::dropEvent(event);
    emit currentRowChanged(currentRow());
}
