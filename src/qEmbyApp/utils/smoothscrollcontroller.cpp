#include "smoothscrollcontroller.h"
#include "wheelinput.h"

#include <QNativeGestureEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>

SmoothScrollController::SmoothScrollController(QScrollBar *scrollBar,
                                               QObject *parent)
    : QObject(parent), m_scrollBar(scrollBar)
{
    if (m_scrollBar) {
        m_targetValue = m_scrollBar->value();

        connect(m_scrollBar, &QObject::destroyed, this, [this]() {
            m_ticker.stop();
            m_scrollBar = nullptr;
        });
        connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
            if (!m_ticker.isActive()) {
                m_targetValue = value;
            }
        });
        connect(m_scrollBar, &QScrollBar::rangeChanged, this,
                [this](int, int) { m_targetValue = boundedValue(m_targetValue); });
    }

    m_ticker.setTimerType(Qt::PreciseTimer);
    m_ticker.setInterval(16);
    connect(&m_ticker, &QTimer::timeout, this, [this]() {
        if (!m_scrollBar) {
            m_ticker.stop();
            return;
        }

        const int current = m_scrollBar->value();
        const int distance = m_targetValue - current;
        if (qAbs(distance) <= 1) {
            m_scrollBar->setValue(m_targetValue);
            m_ticker.stop();
            return;
        }

        qint64 elapsedMs =
            m_frameClock.isValid() ? m_frameClock.restart()
                                   : m_ticker.interval();
        if (elapsedMs <= 0) {
            elapsedMs = m_ticker.interval();
        }
        const qreal frameMs = qMin<qint64>(50, elapsedMs);
        const qreal factor = 1.0 - std::pow(0.08, frameMs / qMax(1, m_durationMs));
        int step = qRound(distance * factor);
        if (step == 0) {
            step = distance > 0 ? 1 : -1;
        }

        m_scrollBar->setValue(current + step);
    });
}

void SmoothScrollController::setDuration(int durationMs)
{
    m_durationMs = qMax(1, durationMs);
}

void SmoothScrollController::setWheelMultiplier(qreal multiplier)
{
    m_wheelMultiplier = qMax(0.1, multiplier);
}

void SmoothScrollController::stop()
{
    m_ticker.stop();
    if (m_scrollBar) {
        m_targetValue = m_scrollBar->value();
    }
}

void SmoothScrollController::scrollTo(int value, bool animated)
{
    if (!m_scrollBar) {
        return;
    }

    m_targetValue = boundedValue(value);
    if (!animated) {
        m_ticker.stop();
        m_scrollBar->setValue(m_targetValue);
        return;
    }

    startTicker();
}

bool SmoothScrollController::scrollByWheelEvent(const QWheelEvent *event,
                                                Qt::Orientation orientation)
{
    if (!m_scrollBar || !event) {
        return false;
    }

    const int wheelValue = wheelDelta(event, orientation);
    if (wheelValue == 0) {
        return false;
    }

    return scrollByDelta(wheelValue, WheelInput::isPrecise(event));
}

bool SmoothScrollController::scrollByNativeGesture(
    const QNativeGestureEvent *event, Qt::Orientation orientation)
{
    if (!event || event->gestureType() != Qt::PanNativeGesture) {
        return false;
    }

    const QPointF value = event->delta();
    const qreal gestureDelta =
        orientation == Qt::Horizontal ? value.x() : value.y();
    return scrollByDelta(gestureDelta, true);
}

bool SmoothScrollController::scrollByDelta(qreal delta, bool precise)
{
    if (!m_scrollBar || qFuzzyIsNull(delta)) {
        return false;
    }

    const int requestedStep = -qRound(delta * m_wheelMultiplier);
    if (requestedStep == 0) {
        return false;
    }

    if (precise) {
        m_ticker.stop();
        const int currentValue = m_scrollBar->value();
        const int nextValue = boundedValue(currentValue + requestedStep);
        m_targetValue = nextValue;
        if (nextValue == currentValue) {
            return false;
        }
        m_scrollBar->setValue(nextValue);
        return true;
    }

    const int currentValue = m_scrollBar->value();
    const int pendingDistance = m_targetValue - currentValue;
    const bool isReversing =
        m_ticker.isActive() && pendingDistance != 0 &&
        ((pendingDistance > 0) != (requestedStep > 0));
    const int baseValue =
        isReversing ? currentValue
                    : (m_ticker.isActive() ? m_targetValue : currentValue);
    const int nextTarget = boundedValue(baseValue + requestedStep);

    // A wheel event at the hard edge belongs to the parent scroll area.  Do
    // not report it as consumed merely because there is no animation running.
    if (!m_ticker.isActive() && nextTarget == currentValue) {
        return false;
    }

    if (nextTarget != m_targetValue || !m_ticker.isActive()) {
        m_targetValue = nextTarget;
        startTicker();
    }
    return true;
}

int SmoothScrollController::targetValue() const
{
    return m_targetValue;
}

int SmoothScrollController::boundedValue(int value) const
{
    if (!m_scrollBar) {
        return value;
    }
    return qBound(m_scrollBar->minimum(), value, m_scrollBar->maximum());
}

int SmoothScrollController::wheelDelta(const QWheelEvent *event,
                                       Qt::Orientation orientation) const
{
    return WheelInput::delta(event, orientation);
}

void SmoothScrollController::startTicker()
{
    if (!m_scrollBar || m_scrollBar->value() == m_targetValue) {
        m_ticker.stop();
        return;
    }

    if (!m_ticker.isActive()) {
        m_frameClock.restart();
        m_ticker.start();
    }
}
