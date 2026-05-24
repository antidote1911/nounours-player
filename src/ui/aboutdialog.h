#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QString version, QString mpvVersion, QWidget *parent = 0);
    ~AboutDialog();

    static void about(QString version, QString mpvVersion, QWidget *parent = 0);

private:
    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
