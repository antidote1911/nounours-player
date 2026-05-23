#include "util.h"
#include "settings.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QGuiApplication>

#include <X11/Xlib.h>

namespace Util {

static Display *x11Display()
{
    static Display *dpy = XOpenDisplay(nullptr);
    return dpy;
}

QString VersionFileUrl()
{
    return "http://nounoursplayer.u8sand.net/version_linux";
}

QString DownloadFileUrl()
{
    return "";
}

bool DimLightsSupported()
{
    if (QGuiApplication::platformName().contains("wayland"))
        return false;
    Display *display = x11Display();
    if (!display)
        return false;
    QString tmp = "_NET_WM_CM_S0";
    Atom a = XInternAtom(display, tmp.toUtf8().constData(), false);
    return (a && XGetSelectionOwner(display, a));
}

void SetAlwaysOnTop(WId wid, bool ontop)
{
    if (QGuiApplication::platformName().contains("wayland"))
        return;
    Display *display = x11Display();
    if (!display)
        return;
    XEvent event = {};
    event.xclient.type         = ClientMessage;
    event.xclient.send_event   = True;
    event.xclient.display      = display;
    event.xclient.window       = wid;
    event.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
    event.xclient.format       = 32;
    event.xclient.data.l[0]   = ontop;
    event.xclient.data.l[1]   = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
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
