#ifndef ASYNCREQUESTGATE_H
#define ASYNCREQUESTGATE_H

#include <QString>
#include <QtGlobal>

class AsyncRequestGate
{
public:
    struct Ticket {
        quint64 generation = 0;
        QString contextKey;
    };

    Ticket begin(const QString& contextKey)
    {
        m_contextKey = contextKey;
        return Ticket{++m_generation, contextKey};
    }

    void invalidate(const QString& contextKey)
    {
        m_contextKey = contextKey;
        ++m_generation;
    }

    bool isCurrent(const Ticket& ticket,
                   const QString& currentContextKey) const
    {
        return ticket.generation == m_generation &&
               ticket.contextKey == m_contextKey &&
               ticket.contextKey == currentContextKey;
    }

private:
    quint64 m_generation = 0;
    QString m_contextKey;
};

#endif
