//
// Created by genin on 26/01/2026.
//

#pragma once

namespace GEF
{
    class Application
    {
    public:
        Application(int argc, char** argv);
        virtual ~Application() = 0;

        void Run();

    private:
        bool is_running_ = true;
    };

    Application* CreateApplication(int argc, char** argv);
}
