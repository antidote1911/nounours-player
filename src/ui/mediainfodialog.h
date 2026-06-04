#pragma once
#include <QDialog>
#include <QString>

class MediaInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MediaInfoDialog(const QString &info, QWidget *parent = nullptr);
    static void show(const QString &info, QWidget *parent = nullptr);
};
