#include "../sylar/log.h"
#include "../sylar/util.h"

int main(int argc, char ** argv) {
    sylar::Logger::ptr logger(new sylar::Logger);
    // logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutAppender));
    // logger->setLevel(sylar::LogLevel::INFO);

    sylar::FileAppender::ptr file_appender(new sylar::FileAppender("./log.txt"));;
    file_appender->setFormatter(sylar::LogFormatter::ptr(new sylar::LogFormatter("%d%T%p%T%m%n")));
    file_appender->setLevel(sylar::LogLevel::ERROR);
    logger->addAppender(file_appender);

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)));
    // event->getSS() << "Hello my log";
    // logger->log(sylar::LogLevel::DEBUG, event);

    SYLAR_LOG_DEBUG(logger) << "Hello logger";
    SYLAR_LOG_ERROR(logger) << "Hello logger error";

    SYLAR_LOG_FMT_ERROR(logger, "Hello logger fmt %s, %d", "haha", 156);

    auto logger_s = sylar::LoggerMgr::getInstance()->getLogger("xxx");
    SYLAR_LOG_ERROR(logger_s) << "xxxxx";
    return 0;
}