#include "mediainfodialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QFont>
#include <QApplication>
#include <QClipboard>

MediaInfoDialog::MediaInfoDialog(const QString &info, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("MediaInfo"));
    resize(800, 600);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    QFont mono("Monospace");
    mono.setStyleHint(QFont::TypeWriter);
    mono.setPointSize(9);
    textEdit->setFont(mono);
    textEdit->setPlainText(info);
    layout->addWidget(textEdit);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    auto *copyBtn = new QPushButton(tr("Copy"), this);
    connect(copyBtn, &QPushButton::clicked, this, [info]{ QApplication::clipboard()->setText(info); });
    btnLayout->addWidget(copyBtn);

    auto *closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
}

void MediaInfoDialog::show(const QString &info, QWidget *parent)
{
    MediaInfoDialog dlg(info, parent);
    dlg.exec();
}
