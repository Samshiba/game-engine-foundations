//
// Created by genin on 08/10/2026.
// Path: Engine/src/Engine/Platform/ImGui/GlfwOpenGLImGuiBackend.cpp
//

#include <Engine/Platform/ImGui/GlfwOpenGLImGuiBackend.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

GEF::Platform::GlfwOpenGLImGuiBackend::GlfwOpenGLImGuiBackend(Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(
        static_cast<GLFWwindow*>(window.GetNativeWindow()), true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

GEF::Platform::GlfwOpenGLImGuiBackend::~GlfwOpenGLImGuiBackend()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GEF::Platform::GlfwOpenGLImGuiBackend::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GEF::Platform::GlfwOpenGLImGuiBackend::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
