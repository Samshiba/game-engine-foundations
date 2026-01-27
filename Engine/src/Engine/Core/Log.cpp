//
// Created by genin on 27/01/2026.
//

#include <Engine/Core/Log.hpp>

namespace GEF
{
    std::shared_ptr<spdlog::logger> Log::EngineLogger_;
    std::shared_ptr<spdlog::logger> Log::ClientLogger_;

    void Log::Init()
    {
        std::vector<spdlog::sink_ptr> sinks;

        sinks.push_back(std::make_shared<
            spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "Engine.log", true));

        sinks[0]->set_level(spdlog::level::debug);
        sinks[1]->set_level(spdlog::level::debug);

        sinks[0]->set_pattern("%^[%T] [%n] {%s:%#} %l%$ - %v");
        sinks[1]->set_pattern("[%T] [%n] {%s:%#} %l - %v");

        EngineLogger_ = std::make_shared<spdlog::logger>(
            "ENGINE", sinks.begin(), sinks.end());
        ClientLogger_ = std::make_shared<spdlog::logger>(
            "APP", sinks.begin(), sinks.end());

        EngineLogger_->flush_on(spdlog::level::debug);
        ClientLogger_->flush_on(spdlog::level::debug);

        EngineLogger_->set_level(spdlog::level::debug);
        ClientLogger_->set_level(spdlog::level::debug);

        spdlog::register_logger(EngineLogger_);
        spdlog::register_logger(ClientLogger_);
    }
}