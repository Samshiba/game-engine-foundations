//
// Created by genin on 27/01/2026.
//

#include <Engine/Core/Log.hpp>

#include "Core.hpp"

namespace GEF
{
#ifdef GEF_ENABLE_LOGGING
    std::shared_ptr<spdlog::logger> Log::EngineLogger_;
    std::shared_ptr<spdlog::logger> Log::ClientLogger_;
#endif

    void Log::Init()
    {
#ifdef GEF_ENABLE_LOGGING
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
        // Fallback
        stdout_pattern = "%^[%T] %n: %v%$";
        level = spdlog::level::info;
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

        EngineLogger_ = std::make_shared<spdlog::logger>(
            "ENGINE", sinks.begin(), sinks.end());
        ClientLogger_ = std::make_shared<spdlog::logger>(
            "APP", sinks.begin(), sinks.end());

        EngineLogger_->flush_on(level);
        ClientLogger_->flush_on(level);

        EngineLogger_->set_level(level);
        ClientLogger_->set_level(level);

        spdlog::register_logger(EngineLogger_);
        spdlog::register_logger(ClientLogger_);
#endif
    }
}