#ifndef WHEELINPUT_H
#define WHEELINPUT_H

#include <QPoint>
#include <Qt>

class QWheelEvent;

namespace WheelInput
{

enum class Axis
{
    None,
    Horizontal,
    Vertical,
};

QPoint effectiveDelta(const QWheelEvent* event);
Axis dominantAxis(const QWheelEvent* event);
int delta(const QWheelEvent* event, Qt::Orientation orientation);
bool isPrecise(const QWheelEvent* event);
bool isPhaseBoundary(const QWheelEvent* event);

class StepAccumulator
{
public:
    int consume(qreal increment,
                Qt::ScrollPhase phase = Qt::NoScrollPhase);
    void reset();

private:
    qreal m_remainder = 0.0;
};

class AxisLock
{
public:
    Axis axisFor(const QWheelEvent* event);
    void reset();

private:
    Axis m_axis = Axis::None;
};

} // namespace WheelInput

#endif
