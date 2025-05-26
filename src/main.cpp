#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

struct ProjectInfo {
    std::string name;
    std::string path;
    std::string type; // "Unity" or "Unreal"
};

std::vector<ProjectInfo> ScanForProjects(const std::string& root) {
    std::vector<ProjectInfo> projects;
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_directory()) {
                // Unity: has Assets and ProjectSettings
                bool hasAssets = std::filesystem::exists(entry.path() / "Assets");
                bool hasSettings = std::filesystem::exists(entry.path() / "ProjectSettings");
                if (hasAssets && hasSettings) {
                    projects.push_back({ entry.path().filename().string(), entry.path().string(), "Unity" });
                    continue;
                }
                // Unreal: has .uproject file
                for (const auto& file : std::filesystem::directory_iterator(entry.path())) {
                    if (file.path().extension() == ".uproject") {
                        projects.push_back({ entry.path().filename().string(), entry.path().string(), "Unreal" });
                        break;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning: " << e.what() << std::endl;
    }
    return projects;
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Project Navigator", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    static char dirBuffer[512] = "C:\\";
    static std::vector<ProjectInfo> projects;
    static bool scanned = false;
    static std::string scanError;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Project Navigator");
        ImGui::Text("Enter the root directory to scan for Unity and Unreal projects:");
        ImGui::InputText("Root Directory", dirBuffer, sizeof(dirBuffer));
        if (ImGui::Button("Scan for Projects")) {
            try {
                projects = ScanForProjects(dirBuffer);
                scanError.clear();
                scanned = true;
            } catch (const std::exception& e) {
                scanError = e.what();
                scanned = false;
            }
        }
        if (!scanError.empty()) {
            ImGui::TextColored(ImVec4(1,0,0,1), "Error: %s", scanError.c_str());
        }
        ImGui::Separator();
        if (scanned) {
            if (projects.empty()) {
                ImGui::Text("No Unity or Unreal projects found.");
            } else {
                ImGui::Text("Found %d projects:", (int)projects.size());
                ImGui::BeginChild("ProjectList", ImVec2(0, 400), true);
                for (size_t i = 0; i < projects.size(); ++i) {
                    const auto& proj = projects[i];
                    ImGui::Text("%s [%s]", proj.name.c_str(), proj.type.c_str());
                    ImGui::SameLine();
                    std::string btnId = "Open##" + std::to_string(i);
                    if (ImGui::Button(btnId.c_str())) {
#ifdef _WIN32
                        std::string cmd = "explorer \"" + proj.path + "\"";
                        system(cmd.c_str());
#else
                        // For other OS, use xdg-open or open
#endif
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
