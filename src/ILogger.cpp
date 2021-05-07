#include "../include/ILogger.h"
#include <fstream>
#include <unordered_map>
#include <map>
#include <fstream>
#include <iostream>

namespace
{
    std::string const defaultLog = "log.txt";

    std::map<RC, std::string const> const  msg =
    {
        {
            RC::ALLOCATION_ERROR,       "error memory allocation"},{
            RC::FILE_NOT_FOUND,         "file not found"}, {
            RC::INDEX_OUT_OF_BOUND,     "index out of bounds"}, {
            RC::INFINITY_OVERFLOW,      "infinity overflow"}, {
            RC::INVALID_ARGUMENT,       "invalid argument"}, {
            RC::MISMATCHING_DIMENSIONS, "mismatching dimensions"}, {
            RC::NOT_NUMBER,             "calculations led to not a number value"}, {
            RC::NULLPTR_ERROR,          "null pointer error"}, {
            RC::SUCCESS,                "success"}, {
            RC::UNKNOWN,                "unknown"}, {
            RC::VECTOR_NOT_FOUND,       "vector not found"}
    };

    std::map<ILogger::Level, std::string const> const levels =
    {
        {
            ILogger::Level::INFO,       "[info:] "   }, {
            ILogger::Level::SEVERE,      "[sever:] "  }, {
            ILogger::Level::WARNING,    "[warning:] "}
    };

    class Logger : public ILogger
    {
        std::ofstream* _log;
    public:
        Logger(std::ofstream* log);
        ~Logger();
        RC log(RC code, Level level, const char* const& srcfile, const char* const& function, int line) override;
        RC log(RC code, Level level) override;
    };
}

ILogger::~ILogger()
{
}

Logger::Logger(std::ofstream* log): _log(log)
{
}

Logger::~Logger()
{
    if (_log->is_open())
        _log->close();

    delete _log;
}

RC Logger::log(RC code, Level level, const char* const& srcfile, const char* const& function, int line)
{
    RC rc = log(code, level);
    if (rc != RC::SUCCESS)
        return rc;

    *_log << "file: " << srcfile << std::endl;
    *_log << "func: " << function << std::endl;
    *_log << "line: " << line << std::endl;

    return RC::SUCCESS;
}

RC Logger::log(RC code, Level level)
{
    auto msgIt = msg.find(code);
    auto levelsIt = levels.find(level);
    if (!_log->is_open() || msgIt == msg.end() || levelsIt == levels.end())
        return RC::UNKNOWN;

    *_log << std::endl;
    *_log << levelsIt->second << msgIt->second << std::endl;

    return RC::SUCCESS;
}

ILogger* ILogger::createLogger(const char* const& filename, bool overwrite)
{
    std::ofstream* log = new std::ofstream;
    if (overwrite)
        log->open(filename);
    else
        log->open(filename, std::fstream::app);

    if (!log->is_open())
        return nullptr;

    return new Logger(log);
}

ILogger* ILogger::createLogger()
{
    return createLogger(defaultLog.c_str());
}



