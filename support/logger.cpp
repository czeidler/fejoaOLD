#include "logger.h"

#include <QDebug>


void LoggerDispatcher::installLogger(ILogger *logger)
{
    fLoggers.append(logger);
}

void LoggerDispatcher::info(const QString &message)
{
    foreach (ILogger *logger, fLoggers)
        logger->info(message);
}

void LoggerDispatcher::warning(const QString &message)
{
    foreach (ILogger *logger, fLoggers)
        logger->warning(message);
}

void LoggerDispatcher::error(const QString &message)
{
    foreach (ILogger *logger, fLoggers)
        logger->error(message);
}


LoggerDispatcher *Log::sLoggerDispatcher = NULL;

void Log::installLogger(ILogger *logger)
{
    getLogger()->installLogger(logger);
}

void Log::info(const QString &message)
{
    getLogger()->info(message);
}

void Log::warning(const QString &message)
{
    getLogger()->warning(message);
}

void Log::error(const QString &message)
{
    getLogger()->error(message);
}

LoggerDispatcher *Log::getLogger()
{
    if (sLoggerDispatcher == NULL)
        sLoggerDispatcher = new LoggerDispatcher();
    return sLoggerDispatcher;
}


void StandardLogger::info(const QString &message)
{
    qDebug() << "Info: " << message;
}

void StandardLogger::warning(const QString &message)
{
    qDebug() << "Warning: " << message;
}

void StandardLogger::error(const QString &message)
{
    qDebug() << "Error: " << message;
}
