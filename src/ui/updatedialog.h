#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QElapsedTimer>


namespace Ui {
class UpdateDialog;
}

class NounoursEngine;

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(NounoursEngine *nounours, QWidget *parent = 0);
    ~UpdateDialog();

    static void CheckForUpdates(NounoursEngine *nounours, QWidget *parent = 0);

protected slots:
    void ShowInfo();

private:
    Ui::UpdateDialog *ui;
    NounoursEngine *nounours;

    QElapsedTimer *timer;
    double avgSpeed = 1,
           lastSpeed = 0;
    int lastProgress,
        state;
    qint64 lastTime;
    bool init;
};

#endif // UPDATEDIALOG_H
