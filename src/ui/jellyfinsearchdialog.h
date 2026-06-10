#ifndef JELLYFINSEARCHDIALOG_H
#define JELLYFINSEARCHDIALOG_H

#include <QDialog>

#include "../jellyfinmanager.h"

class NounoursEngine;
class JellyfinResultsWidget;

class QLineEdit;
class QPushButton;
class QListWidget;
class QLabel;

class JellyfinSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit JellyfinSearchDialog(NounoursEngine *nounours, QWidget *parent = nullptr);

private:
    NounoursEngine  *nounours;
    JellyfinManager *jellyfin;

    QLineEdit   *searchEdit;
    QPushButton *searchButton;
    QListWidget *genresList;
    QLabel      *statusLabel;

    JellyfinResultsWidget *searchResults;
    JellyfinResultsWidget *genreResults;

    void DoSearch();
};

#endif // JELLYFINSEARCHDIALOG_H
