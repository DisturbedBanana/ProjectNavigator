#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
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

std::string GetConfigPath() {
    std::string configPath;
#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        configPath = std::string(appDataPath) + "\\ProjectNavigator";
        std::filesystem::create_directories(configPath);
        configPath += "\\config.txt";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        configPath = std::string(home) + "/.config/ProjectNavigator";
        std::filesystem::create_directories(configPath);
        configPath += "/config.txt";
    }
#endif
    return configPath;
}

std::string LoadLastDirectory() {
    std::string configPath = GetConfigPath();
    if (std::filesystem::exists(configPath)) {
        std::ifstream file(configPath);
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) {
                return line;
            }
        }
    }
    return "C:\\"; // Default directory
}

void SaveLastDirectory(const std::string& directory) {
    std::string configPath = GetConfigPath();
    std::ofstream file(configPath);
    if (file.is_open()) {
        file << directory;
    }
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
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

    // Load the last used directory
    std::string lastDirectory = LoadLastDirectory();
    static char dirBuffer[512];
    strncpy(dirBuffer, lastDirectory.c_str(), sizeof(dirBuffer) - 1);
    dirBuffer[sizeof(dirBuffer) - 1] = '\0';

    static std::vector<ProjectInfo> projects;
    static bool scanned = false;
    static std::string scanError;

    // Perform initial scan
    try {
        projects = ScanForProjects(dirBuffer);
        scanError.clear();
        scanned = true;
    } catch (const std::exception& e) {
        scanError = e.what();
        scanned = false;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Set window to take full size
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Project Navigator", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Enter the root directory to scan for Unity and Unreal projects:");
        ImGui::InputText("Root Directory", dirBuffer, sizeof(dirBuffer));
        if (ImGui::Button("Scan for Projects")) {
            try {
                projects = ScanForProjects(dirBuffer);
                scanError.clear();
                scanned = true;
                // Save the directory when scanning is successful
                SaveLastDirectory(dirBuffer);
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
                // Split window into two columns
                ImGui::Columns(2, "ProjectColumns", true);
                
                // Unity Projects Column
                ImGui::Text("Unity Projects");
                ImGui::Separator();
                ImGui::BeginChild("UnityProjects", ImVec2(0, ImGui::GetContentRegionAvail().y), true);
                bool hasUnityProjects = false;
                for (const auto& proj : projects) {
                    if (proj.type == "Unity") {
                        hasUnityProjects = true;
                        ImGui::Text("%s", proj.name.c_str());
                        ImGui::SameLine();
                        std::string btnId = "Open##Unity" + proj.name;
                        if (ImGui::Button(btnId.c_str())) {
#ifdef _WIN32
                            std::string cmd = "explorer \"" + proj.path + "\"";
                            system(cmd.c_str());
#else
                            // For other OS, use xdg-open or open
#endif
                        }
                    }
                }
                if (!hasUnityProjects) {
                    ImGui::Text("No Unity projects found");
                }
                ImGui::EndChild();

                // Move to next column
                ImGui::NextColumn();

                // Unreal Projects Column
                ImGui::Text("Unreal Projects");
                ImGui::Separator();
                ImGui::BeginChild("UnrealProjects", ImVec2(0, ImGui::GetContentRegionAvail().y), true);
                bool hasUnrealProjects = false;
                for (const auto& proj : projects) {
                    if (proj.type == "Unreal") {
                        hasUnrealProjects = true;
                        ImGui::Text("%s", proj.name.c_str());
                        ImGui::SameLine();
                        std::string btnId = "Open##Unreal" + proj.name;
                        if (ImGui::Button(btnId.c_str())) {
#ifdef _WIN32
                            std::string cmd = "explorer \"" + proj.path + "\"";
                            system(cmd.c_str());
#else
                            // For other OS, use xdg-open or open
#endif
                        }
                    }
                }
                if (!hasUnrealProjects) {
                    ImGui::Text("No Unreal projects found");
                }
                ImGui::EndChild();

                ImGui::Columns(1);
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
