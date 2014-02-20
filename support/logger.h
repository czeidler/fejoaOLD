#ifndef LOGGER_H
#define LOGGER_H

#include <QList>

class ILogger {
public:
    virtual ~ILogger() {}

    virtual void info(const QString &message) = 0;
    virtual void warning(const QString &message) = 0;
    virtual void error(const QString &message) = 0;
};


class LoggerDispatcher : public ILogger
{
public:
    void installLogger(ILogger *logger);

    virtual void info(const QString &message);
    virtual void warning(const QString &message);
    virtual void error(const QString &message);

private:
    QList<ILogger*> fLoggers;
};

// singleton + decorator
class Log {
public:
    static void installLogger(ILogger *logger);

    static void info(const QString &message);
    static void warning(const QString &message);
    static void error(const QString &message);

private:
    static LoggerDispatcher *getLogger();

    static LoggerDispatcher *sLoggerDispatcher;
};

class StandardLogger {
public:
    virtual void info(const QString &message);
    virtual void warning(const QString &message);
    virtual void error(const QString &message);
};

#endif // LOGGER_H
