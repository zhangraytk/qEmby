#ifndef SMOOTHSCROLLCONTROLLER_H
#define SMOOTHSCROLLCONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QtGlobal>

class QScrollBar;
class QNativeGestureEvent;
class QWheelEvent;

class SmoothScrollController : public QObject
{
    Q_OBJECT
public:
    explicit SmoothScrollController(QScrollBar *scrollBar,
                                    QObject *parent = nullptr);

    void setDuration(int durationMs);
    void setWheelMultiplier(qreal multiplier);
    void stop();
    void scrollTo(int value, bool animated = true);
    bool scrollByWheelEvent(const QWheelEvent *event,
                            Qt::Orientation orientation);
    bool scrollByNativeGesture(const QNativeGestureEvent *event,
                               Qt::Orientation orientation);

    int targetValue() const;

private:
    int boundedValue(int value) const;
    int wheelDelta(const QWheelEvent *event,
                   Qt::Orientation orientation) const;
    bool scrollByDelta(qreal delta, bool precise);
    void startTicker();

    QScrollBar *m_scrollBar = nullptr;
    QTimer m_ticker;
    QElapsedTimer m_frameClock;
    int m_targetValue = 0;
    int m_durationMs = 160;
    qreal m_wheelMultiplier = 1.0;
};

#endif 
