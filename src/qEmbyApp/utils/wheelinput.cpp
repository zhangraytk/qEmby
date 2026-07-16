#include "wheelinput.h"

#include <QInputDevice>
#include <QWheelEvent>

namespace WheelInput
{

QPoint effectiveDelta(const QWheelEvent* event)
{
    if (!event) {
        return {};
    }

    const QPoint pixels = event->pixelDelta();
    return pixels.isNull() ? event->angleDelta() : pixels;
}

Axis dominantAxis(const QWheelEvent* event)
{
    const QPoint value = effectiveDelta(event);
    const int x = qAbs(value.x());
    const int y = qAbs(value.y());
    if (x == 0 && y == 0) {
        return Axis::None;
    }
    return x > y ? Axis::Horizontal : Axis::Vertical;
}

int delta(const QWheelEvent* event, Qt::Orientation orientation)
{
    const QPoint value = effectiveDelta(event);
    return orientation == Qt::Horizontal ? value.x() : value.y();
}

bool isPrecise(const QWheelEvent* event)
{
    if (!event) {
        return false;
    }

    if (!event->pixelDelta().isNull()) {
        return true;
    }

    const QInputDevice* device = event->device();
    return device && device->type() == QInputDevice::DeviceType::TouchPad;
}

bool isPhaseBoundary(const QWheelEvent* event)
{
    return event && (event->phase() == Qt::ScrollBegin ||
                     event->phase() == Qt::ScrollEnd);
}

int StepAccumulator::consume(qreal increment, Qt::ScrollPhase phase)
{
    if (phase == Qt::ScrollBegin) {
        reset();
    }

    int steps = 0;
    if (!qFuzzyIsNull(increment)) {
        if (!qFuzzyIsNull(m_remainder) &&
            ((m_remainder > 0.0) != (increment > 0.0))) {
            reset();
        }

        m_remainder += increment;
        const qreal rounded = qRound(m_remainder);
        steps = qFuzzyIsNull(m_remainder - rounded)
            ? static_cast<int>(rounded)
            : static_cast<int>(m_remainder);
        if (steps != 0) {
            m_remainder -= steps;
        }
    }

    if (phase == Qt::ScrollEnd) {
        reset();
    }
    return steps;
}

void StepAccumulator::reset()
{
    m_remainder = 0.0;
}

Axis AxisLock::axisFor(const QWheelEvent* event)
{
    if (!event) {
        return Axis::None;
    }

    const Qt::ScrollPhase phase = event->phase();
    if (phase == Qt::ScrollBegin) {
        m_axis = Axis::None;
    }

    const Axis candidate = dominantAxis(event);
    if (phase == Qt::NoScrollPhase) {
        return candidate;
    }

    if (m_axis == Axis::None && candidate != Axis::None) {
        m_axis = candidate;
    }

    const Axis result = m_axis;
    if (phase == Qt::ScrollEnd) {
        m_axis = Axis::None;
    }
    return result;
}

void AxisLock::reset()
{
    m_axis = Axis::None;
}

} // namespace WheelInput
