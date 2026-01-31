//
// Created by genin on 27/01/2026.
//

#pragma once

#ifdef GEF_ENABLE_LOGGING
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>
#endif

namespace GEF
{
    class Log
    {
    public:
        static void Init();

#ifdef GEF_ENABLE_LOGGING
        static std::shared_ptr<spdlog::logger>& GetEngineLogger();

        static std::shared_ptr<spdlog::logger>& GetClientLogger();
#endif

    private:
#ifdef GEF_ENABLE_LOGGING
        static std::shared_ptr<spdlog::logger> EngineLogger_;
        static std::shared_ptr<spdlog::logger> ClientLogger_;
#endif
    };
}

#ifdef GEF_ENABLE_LOGGING

inline std::shared_ptr<spdlog::logger>& GEF::Log::GetEngineLogger()
{
    return EngineLogger_;
}

inline std::shared_ptr<spdlog::logger>& GEF::Log::GetClientLogger()
{
    return ClientLogger_;
}

#define GEF_ENGINE_DEBUG(...)    SPDLOG_LOGGER_CALL(GEF::Log::GetEngineLogger(), spdlog::level::debug, __VA_ARGS__)
#define GEF_ENGINE_INFO(...)     SPDLOG_LOGGER_CALL(GEF::Log::GetEngineLogger(), spdlog::level::info, __VA_ARGS__)
#define GEF_ENGINE_WARN(...)     SPDLOG_LOGGER_CALL(GEF::Log::GetEngineLogger(), spdlog::level::warn, __VA_ARGS__)
#define GEF_ENGINE_ERROR(...)    SPDLOG_LOGGER_CALL(GEF::Log::GetEngineLogger(), spdlog::level::err, __VA_ARGS__)
#define GEF_ENGINE_CRITICAL(...) SPDLOG_LOGGER_CALL(GEF::Log::GetEngineLogger(), spdlog::level::critical, __VA_ARGS__)

#define GEF_DEBUG(...)           SPDLOG_LOGGER_CALL(GEF::Log::GetClientLogger(), spdlog::level::debug, __VA_ARGS__)
#define GEF_INFO(...)            SPDLOG_LOGGER_CALL(GEF::Log::GetClientLogger(), spdlog::level::info, __VA_ARGS__)
#define GEF_WARN(...)            SPDLOG_LOGGER_CALL(GEF::Log::GetClientLogger(), spdlog::level::warn, __VA_ARGS__)
#define GEF_ERROR(...)           SPDLOG_LOGGER_CALL(GEF::Log::GetClientLogger(), spdlog::level::err, __VA_ARGS__)
#define GEF_CRITICAL(...)        SPDLOG_LOGGER_CALL(GEF::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)

#else

#define GEF_ENGINE_DEBUG(...)    (void)0
#define GEF_ENGINE_INFO(...)     (void)0
#define GEF_ENGINE_WARN(...)     (void)0
#define GEF_ENGINE_ERROR(...)    (void)0
#define GEF_ENGINE_CRITICAL(...) (void)0

#define GEF_DEBUG(...)           (void)0
#define GEF_INFO(...)            (void)0
#define GEF_WARN(...)            (void)0
#define GEF_ERROR(...)           (void)0
#define GEF_CRITICAL(...)        (void)0

#endif