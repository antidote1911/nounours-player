#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

class NounoursEngine;

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QString file, QObject *parent = 0);
    ~Settings();

public slots:
    void Load();
    void Save();

    QJsonObject getRoot();
    void setRoot(QJsonObject);

protected:
    void LoadIni();
    int ParseLine(QString line);
    QString FixKeyOnLoad(QString key);
    QStringList SplitQStringList(QString list);

private:
    NounoursEngine *nounours;
    QJsonDocument document;
    QString file;
};

#endif // SETTINGS_H
