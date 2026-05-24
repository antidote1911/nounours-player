#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QDesktopServices>
#include <QUrl>

AboutDialog::AboutDialog(QString version, QString mpvVersion, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->versionLabel->setText("Nounours-Player "+version);
    ui->mpvVersionLabel->setText(mpvVersion);

    connect(ui->infoLabel, &QLabel::linkActivated,
            [](const QString &url) { QDesktopServices::openUrl(QUrl(url)); });

    connect(ui->closeButton, SIGNAL(clicked()),
            this, SLOT(close()));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::about(QString version, QString mpvVersion, QWidget *parent)
{
    AboutDialog dialog(version, mpvVersion, parent);
    dialog.exec();
}
