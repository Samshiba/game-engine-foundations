//
// Created by genin on 27/01/2026.
// Path: Engine/src/Engine/Core/Log.cpp
//

#include <Engine/Core/Log.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>

namespace GEF
{
#ifdef GEF_ENABLE_LOGGING
    namespace
    {
        struct Loggers
        {
            std::shared_ptr<spdlog::logger> engine;
            std::shared_ptr<spdlog::logger> client;
        };

        Loggers CreateLoggers()
        {
            std::string stdout_pattern;
            std::string file_pattern;
            spdlog::level::level_enum level;

#if defined(GEF_DEBUG_BUILD)
            stdout_pattern = "%^[%T] [%n] {%s:%#} %l%$ - %v";
            file_pattern = "[%T] [%n] {%s:%#} %l - %v";
            level = spdlog::level::debug;

#elif defined(GEF_PROFILE_BUILD)
            stdout_pattern = "%^[%T] [%n] %l%$ - %v";
            file_pattern = "[%T] [%n] %l - %v";
            level = spdlog::level::info;

#else
            // Release: only warnings and errors
            stdout_pattern = "%^[%T] [%n] %l%$ - %v";
            file_pattern = "[%T] [%n] %l - %v";
            level = spdlog::level::warn;
#endif

            std::vector<spdlog::sink_ptr> sinks;

            sinks.emplace_back(std::make_shared<
                spdlog::sinks::stdout_color_sink_mt>());
            sinks.emplace_back(
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    "Engine.log", true));

            sinks[0]->set_level(level);
            sinks[1]->set_level(level);

            sinks[0]->set_pattern(stdout_pattern);
            sinks[1]->set_pattern(file_pattern);

            auto engine = std::make_shared<spdlog::logger>(
                "ENGINE", sinks.begin(), sinks.end());
            auto client = std::make_shared<spdlog::logger>(
                "CLIENT", sinks.begin(), sinks.end());

            engine->flush_on(level);
            client->flush_on(level);

            engine->set_level(level);
            client->set_level(level);

            spdlog::register_logger(engine);
            spdlog::register_logger(client);

            return { engine, client };
        }

        Loggers& GetLoggers()
        {
            static Loggers loggers = CreateLoggers();
            return loggers;
        }
    }

    std::shared_ptr<spdlog::logger>& Log::GetEngineLogger()
    {
        return GetLoggers().engine;
    }

    std::shared_ptr<spdlog::logger>& Log::GetClientLogger()
    {
        return GetLoggers().client;
    }
#endif

    void Log::Init()
    {
#ifdef GEF_ENABLE_LOGGING
        (void)GetLoggers();
#endif
    }
}