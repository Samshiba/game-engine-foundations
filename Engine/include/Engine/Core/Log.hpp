//
// Created by genin on 27/01/2026.
//

#pragma once

#include <Core.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


namespace GEF
{
    class Log
    {
    public:
        static void Init();

        static std::shared_ptr<spdlog::logger>& GetEngineLogger();

        static std::shared_ptr<spdlog::logger>& GetClientLogger();

    private:
        static std::shared_ptr<spdlog::logger> EngineLogger_;
        static std::shared_ptr<spdlog::logger> ClientLogger_;
    };
}

inline std::shared_ptr<spdlog::logger>& GEF::Log::GetEngineLogger()
{
    return EngineLogger_;
}

inline std::shared_ptr<spdlog::logger>& GEF::Log::GetClientLogger()
{
    return ClientLogger_;
}

#define GEF_ENGINE_DEBUG(...)       GEF::Log::GetEngineLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#define GEF_ENGINE_INFO(...)        GEF::Log::GetEngineLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define GEF_ENGINE_WARN(...)        GEF::Log::GetEngineLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define GEF_ENGINE_ERROR(...)       GEF::Log::GetEngineLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::error, __VA_ARGS__)
#define GEF_ENGINE_CRITICAL(...)    GEF::Log::GetEngineLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__)

#define GEF_DEBUG(...)              GEF::Log::GetClientLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#define GEF_INFO(...)               GEF::Log::GetClientLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define GEF_WARN(...)               GEF::Log::GetClientLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define GEF_ERROR(...)              GEF::Log::GetClientLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::error, __VA_ARGS__)
#define GEF_CRITICAL(...)           GEF::Log::GetClientLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__)
