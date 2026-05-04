#include <vector>
#include <iostream>
#include <format>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "SteamAccInfo.hpp"
#include "Games.hpp"

struct WindowStatus
{
    int currentGameIdx = 0;
};


int main(void)
{
    if(!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(800, 600, "rzstmcfg", nullptr, nullptr);
    if(!window)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto steamAccs = SteamAccInfo::GetAllAccounts();

    WindowStatus status;

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGui::Begin("A", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        const auto& gameInfos = getAllGameInfos();
        
        if(ImGui::BeginCombo("Game", getGameNameCStr(gameInfos[status.currentGameIdx]), ImGuiComboFlags_HeightLarge))
        {
            int n = 0;
            for(auto gameinfo : gameInfos)
            {
                const bool selected = status.currentGameIdx == n;
                if(ImGui::Selectable(getGameNameCStr(gameinfo), selected))
                    status.currentGameIdx = n;

                if(selected)
                    ImGui::SetItemDefaultFocus();
                n++;
            }
            ImGui::EndCombo();
        }

        for(const auto& acc : steamAccs)
        {
            ImGui::Text("%s", acc.uname.c_str());
        }

        ImGui::End();

        ImGui::Render();
        int dis_w, dis_h;
        dis_w = dis_h = 0;
        glfwGetFramebufferSize(window, &dis_w, &dis_h);
        glViewport(0, 0, dis_w, dis_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Fix for wayland segfault
    glfwPollEvents();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}