#include "overlayhandler.h"

#include "nounoursengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "overlay.h"

#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QBrush>
#include <QTimer>
#include <QFontMetrics>
#include <QThread>

#define OVERLAY_INFO    62
#define OVERLAY_STATUS  63
// mpv's overlay_add only accepts ids 0-63, so use a free slot below the
// auto-assigned pool (1-60) rather than 64, which mpv silently rejects
#define OVERLAY_MESSAGE 61
#define OVERLAY_REFRESH_RATE 1000

OverlayHandler::OverlayHandler(QObject *parent):
    QObject(parent),
    nounours(static_cast<NounoursEngine*>(parent)),
    refresh_timer(nullptr),
    min_overlay(1),
    max_overlay(60),
    overlay_id(min_overlay)
{
}

OverlayHandler::~OverlayHandler()
{
    for(auto o : overlays)
        delete o;
}

void OverlayHandler::showStatusText(const QString &text, int duration)
{
    if(text != QString())
        showText(text,
                 QFont(Util::MonospaceFont(),
                       14, QFont::Bold), QColor(0xFFFFFF),
                 QPoint(20, 20), duration, OVERLAY_STATUS);
    else if(duration == 0)
        remove(OVERLAY_STATUS);
}

void OverlayHandler::showMessage(const QString &text, int duration)
{
    // uses its own overlay id so it isn't cleared early by the
    // buffering/pause status overlay (OVERLAY_STATUS)
    if(text != QString())
    {
        QFont font(Util::MonospaceFont(), 14, QFont::Bold);
        QPoint pos(20, 20);

        // wrap onto multiple lines instead of letting showText shrink the
        // font to fit everything on one line
        const float fm_correction = 1.3;
        QFontMetrics fm(font);
        int availW = nounours->window->ui->mpvFrame->width() - 2*pos.x();

        QStringList wrapped;
        for(const QString &line : text.split('\n'))
        {
            QString current;
            for(const QString &word : line.split(' '))
            {
                QString candidate = current.isEmpty() ? word : current + ' ' + word;
                if(!current.isEmpty() && fm.horizontalAdvance(candidate)*fm_correction > availW)
                {
                    wrapped << current;
                    current = word;
                }
                else
                    current = candidate;
            }
            wrapped << current;
        }

        showText(wrapped.join('\n'), font, QColor(0xFFFFFF), pos, duration, OVERLAY_MESSAGE, 0, true);
    }
    else if(duration == 0)
        remove(OVERLAY_MESSAGE);
}

void OverlayHandler::showInfoText(bool show)
{
    if(show)
    {
        if(refresh_timer == nullptr)
        {
            refresh_timer = new QTimer(this);
            refresh_timer->setSingleShot(true);
            connect(refresh_timer, &QTimer::timeout, [=] { showInfoText(); });
        }
        refresh_timer->start(OVERLAY_REFRESH_RATE);
        showText(nounours->mpv->getMediaInfo(),
                 QFont(Util::MonospaceFont(), 11, QFont::Bold), QColor(0xFFFF00),
                 QPoint(20, 20), 0, OVERLAY_INFO);
    }
    else
    {
        delete refresh_timer;
        refresh_timer = nullptr;
        remove(OVERLAY_INFO);
    }
}

void OverlayHandler::showText(const QString &text, QFont font, QColor color, QPoint pos, int duration, int id, int maxWidth, bool noShrink)
{
    overlay_mutex.lock();
    // increase next overlay_id
    if(id == -1) // auto id
    {
        id = overlay_id;
        if(overlay_id+1 > max_overlay)
            overlay_id = min_overlay;
        else
            ++overlay_id;
    }

    QFontMetrics fm(font);
    QStringList lines = text.split('\n');
    // the 1.3 was pretty much determined through trial and error; this formula isn't perfect
    // apparently, QFontMetrics doesn't work that well
    const float fm_correction = 1.3;
    int w = 0,
        h = fm.height()*lines.length();
    for(auto line : lines)
        w = std::max(fm.horizontalAdvance(line), w);
    int availW = maxWidth > 0 ? maxWidth : nounours->window->ui->mpvFrame->width() - 2*pos.x();
    int availH = nounours->window->ui->mpvFrame->height() - 2*pos.y();
    if(id == OVERLAY_INFO) {
        font.setPointSizeF(nounours->window->ui->mpvFrame->height() / 45.0);
    } else if(!noShrink) {
        float xF = float(availW) / (fm_correction*w);
        float yF = float(availH) / h;
        font.setPointSizeF(std::min(font.pointSizeF()*std::min(xF, yF), font.pointSizeF()));
    }

    fm = QFontMetrics(font);
    h = fm.height();
    w = 0;
    QPainterPath path(QPoint(0, 0));
    QPoint p = QPoint(0, h);
    for(auto line : lines)
    {
        path.addText(p, font, line);
        w = std::max(int(fm_correction*path.currentPosition().x()), w);
        p += QPoint(0, h);
    }

    QImage *canvas = new QImage(w, p.y(), QImage::Format_ARGB32);
    canvas->fill(QColor(0,0,0,0)); // fill it with nothing

    QPainter painter(canvas); // prepare to paint
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setFont(font);
    // halo all around the text
    QPainterPathStroker stroker;
    stroker.setWidth(5.0);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 210));
    painter.drawPath(stroker.createStroke(path));
    // text pass
    painter.setBrush(color);
    painter.drawPath(path);

    // add as mpv overlay
    nounours->mpv->AddOverlay(
        id == -1 ? overlay_id : id,
        pos.x(), pos.y(),
        "&"+QString::number(quintptr(canvas->bits())),
        0, canvas->width(), canvas->height());

    // add over mpv as label
    QLabel *label = new QLabel(nounours->window->ui->mpvFrame);
    label->setStyleSheet("background-color:rgb(0,0,0,0);background-image:url();");
    label->setGeometry(pos.x(),
                       pos.y(),
                       canvas->width(),
                       canvas->height());
    label->setPixmap(QPixmap::fromImage(*canvas));
    label->show();

    QTimer *timer;
    if(duration == 0)
        timer = nullptr;
    else
    {
        timer = new QTimer(this);
        timer->start(duration);
        connect(timer, &QTimer::timeout, // on timeout
                [=] { remove(id); });
    }

    if(overlays.find(id) != overlays.end())
        delete overlays[id];
    overlays[id] = new Overlay(label, canvas, timer, this);
    overlay_mutex.unlock();
}

void OverlayHandler::remove(int id)
{
    overlay_mutex.lock();
    nounours->mpv->RemoveOverlay(id);
    if(overlays.find(id) != overlays.end())
    {
        delete overlays[id];
        overlays.remove(id);
    }
    overlay_mutex.unlock();
}
