#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QString>
#include <QPair>
#include <QMutex>
#include <QTableWidget>

namespace Ui {
class PreferencesDialog;
}

class NounoursEngine;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(NounoursEngine *nounours, QWidget *parent = 0);
    ~PreferencesDialog();

    static void showPreferences(NounoursEngine *nounours, QWidget *parent = 0);

protected:
    void PopulateLangs();
    void PopulateShortcuts();
    void AddRow(QString first, QString second, QString third);
    void ModifyRow(int row, QString first, QString second, QString third);
    void RemoveRow(int row);
    void SelectKey(bool add, QPair<QString, QPair<QString, QString>> init = (QPair<QString, QPair<QString, QString>>()));
    void PopulateVoList();
    void UpdateVoDescription(const QString &vo);
    void PopulateHwdecList();
    void UpdateHwdecDescription(const QString &hwdec);

private:
    Ui::PreferencesDialog *ui;
    NounoursEngine *nounours;
    QHash<QString, QPair<QString, QString>> saved;

    QString screenshotDir;
    int numberOfShortcuts;

    class SortLock : public QMutex
    {
    public:
        SortLock(QTableWidget *parent);

        void lock();
        void unlock();
    private:
        QTableWidget *parent;
    } *sortLock;
};

#endif // PREFERENCESDIALOG_H
