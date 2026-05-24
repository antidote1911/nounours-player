#include "seekbar.h"

#include <QTime>
#include <QToolTip>
#include <QPainter>
#include <QRect>
#include <QStyle>

#include "util.h"

SeekBar::SeekBar(QWidget *parent):
    CustomSlider(parent),
    tickReady(false),
    totalTime(0),
    tooltipTimer(new QTimer(this)),
    tooltipTime(0)
{
    tooltipTimer->setSingleShot(true);
    connect(tooltipTimer, &QTimer::timeout, this, [this]()
    {
        QToolTip::showText(tooltipPos,
                           Util::FormatTime(tooltipTime, totalTime),
                           this, rect());
    });
}

void SeekBar::setTracking(int _totalTime)
{
    if(_totalTime != 0)
    {
        totalTime = _totalTime;
        // now that we've got totalTime, calculate the tick locations
        // we need to do this because totalTime is obtained after the LOADED event is fired--we need totalTime for calculations
        for(auto &tick : ticks)
            tick = ((double)tick/totalTime)*maximum();
        if(ticks.length() > 0)
        {
            tickReady = true; // ticks are ready to be displayed
            repaint(rect());
        }
        setMouseTracking(true);
    }
    else
        setMouseTracking(false);
}

void SeekBar::setTicks(QList<int> values)
{
    ticks = values; // just set the values
    tickReady = false; // ticks need to be converted when totalTime is obtained
}

void SeekBar::mousePressEvent(QMouseEvent* event)
{
    tooltipTimer->stop();
    QToolTip::hideText();
    CustomSlider::mousePressEvent(event);
}

void SeekBar::mouseMoveEvent(QMouseEvent* event)
{
    if(totalTime != 0)
    {
        int t = QStyle::sliderValueFromPosition(minimum(), maximum(),
            qRound(event->position().x()), width()) * (double)totalTime / maximum();
        if(event->buttons() & Qt::LeftButton)
        {
            tooltipTimer->stop();
            QToolTip::hideText();
        }
        else
        {
            tooltipTime = t;
            tooltipPos  = QPoint(qRound(event->globalPosition().x()) - 25,
                                 mapToGlobal(rect().topLeft()).y() - 70);
            tooltipTimer->start(500);
        }
    }
    QSlider::mouseMoveEvent(event);
}

void SeekBar::mouseReleaseEvent(QMouseEvent* event)
{
    QSlider::mouseReleaseEvent(event);
}

void SeekBar::leaveEvent(QEvent* event)
{
    tooltipTimer->stop();
    QToolTip::hideText();
    QSlider::leaveEvent(event);
}

void SeekBar::paintEvent(QPaintEvent *event)
{
    CustomSlider::paintEvent(event);
    if(isEnabled() && tickReady)
    {
        QRect region = event->rect();
        QPainter painter(this);
        painter.setPen(QColor(0,0,0));
        int mid = region.center().y();
        for(auto &tick : ticks)
        {
            int x = QStyle::sliderPositionFromValue(minimum(), maximum(), tick, width());
            painter.drawLine(x, mid - 4, x, mid + 4);
        }
    }
}
