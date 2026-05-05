#include <vector>
#include <iostream>
#include <format>
#include <ranges>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "SteamAccInfo.hpp"
#include "Games.hpp"
#include "Borrower.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

struct WindowStatus
{
    int currentGameIdx = 0;
    const SteamAccInfo *selectedFromAcc = nullptr;
    const SteamAccInfo *selectedToAcc = nullptr;
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
    auto& style = ImGui::GetStyle();

    style.ScaleAllSizes(3.0f);

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto steamAccs = SteamAccInfo::GetAllAccounts();
    auto& borrower = Borrower::GetBorrower();

    WindowStatus status;
    const auto& gameInfos = getAllGameInfos();

    auto gameDirFilter = [&gameInfos, &status](const SteamAccInfo& acc) -> bool{
        return acc.hasGameDataDir(gameInfos[status.currentGameIdx].get().id);
    };

    auto selectedToFilter = [&status](const SteamAccInfo& acc) -> bool{
        return status.selectedFromAcc != &acc;
    };

    auto drawDownArrow = []() {
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        float arrow_width  = 14.0f;
        float arrow_height = 10.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos     = ImGui::GetCursorScreenPos(); 
        float avail_width     = ImGui::GetContentRegionAvail().x;
        float center_x        = cursor_pos.x + (avail_width / 2.0f) - (arrow_width / 2);
        float y               = cursor_pos.y - (arrow_height / 2.0f);
        

        ImVec2 p1(center_x - arrow_width / 2.0f, y);
        ImVec2 p2(center_x + arrow_width / 2.0f, y); 
        ImVec2 p3(center_x, y + arrow_height);
        
        draw_list->AddTriangleFilled(p1, p2, p3, ImGui::GetColorU32(ImGuiCol_Text));

        ImGui::Dummy(ImVec2(0.0f, arrow_height + 5.0f));
    };

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGui::Begin("A", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        
        if(ImGui::BeginCombo("##GameCombo", getGameNameCStr(gameInfos[status.currentGameIdx]), ImGuiComboFlags_HeightLarge))
        {
            int n = 0;
            for(auto gameinfo : gameInfos)
            {
                const bool selected = status.currentGameIdx == n;
                if(ImGui::Selectable(getGameNameCStr(gameinfo), selected)){
                    status.selectedToAcc = status.selectedFromAcc = nullptr;
                    status.currentGameIdx = n;
                }

                if(selected)
                    ImGui::SetItemDefaultFocus();
                n++;
            }
            ImGui::EndCombo();
        }

        if(ImGui::BeginTable("From Account Table", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("From Account");
            ImGui::TableHeadersRow();
            for(const auto& acc : steamAccs | std::views::filter(gameDirFilter))
            {
                ImGui::TableNextColumn();
                const bool selected = status.selectedFromAcc == &acc;
                auto lbl = std::format("{}##{}", acc.uname, acc.id64);
                if(ImGui::Selectable(lbl.c_str(), selected)){
                    status.selectedFromAcc = selected ? nullptr : &acc;
                    status.selectedToAcc = nullptr;
                }
            }
            ImGui::EndTable();
        }

        drawDownArrow();
        
        if(ImGui::BeginTable("To Account Table", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("To Account");
            ImGui::TableHeadersRow();

            for(const auto& acc : steamAccs | std::views::filter(gameDirFilter) | std::views::filter(selectedToFilter))
            {
                ImGui::TableNextColumn();
                const bool selected = status.selectedToAcc == &acc;
                auto lbl = std::format("{}##{}", acc.uname, acc.id64);
                if(ImGui::Selectable(lbl.c_str(), selected)){
                    status.selectedToAcc = selected ? nullptr : &acc;
                }
            }
            ImGui::EndTable();
        }

        if(ImGui::Button("Return")){

        }

        ImGui::SameLine();

        if(ImGui::Button("Borrow")){

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

#ifdef _WIN32
int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
)
{
    return main();
}
#endif
