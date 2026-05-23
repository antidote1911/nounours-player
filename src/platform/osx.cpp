#include "util.h"
#include "settings.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

namespace Util {

QString VersionFileUrl()
{
    return "http://nounoursplayer.u8sand.net/version_osx";
}

QString DownloadFileUrl()
{
    return "";
}

bool DimLightsSupported()
{
    return true;
}

void SetAlwaysOnTop(WId wid, bool ontop)
{
    Q_UNUSED(wid)
    Q_UNUSED(ontop)
}

QString SettingsLocation()
{
    return QString("%1/%2.ini").arg(
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
        SETTINGS_FILE);
}

bool IsValidFile(QString path)
{
    return QRegularExpression("^\\.{1,2}|/").match(path).hasMatch();
}

bool IsValidLocation(QString loc)
{
    return QRegularExpression("^([a-z]{2,}://|\\.{1,2}|/)").match(loc).hasMatch();
}

void ShowInFolder(QString path, QString)
{
    QDesktopServices::openUrl(QString("file:///%1").arg(path));
}

QString MonospaceFont()
{
    return "Monospace";
}

}
