#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

// Add at file scope (top of file, after includes):
#ifdef _WIN32
static bool g_draggingWindow = false;
static ImVec2 g_clickOffset;
#endif

struct ProjectInfo {
    std::string name;
    std::string path;
    std::string type; // "Unity" or "Unreal"
};

struct UISettings {
    // Colors
    ImVec4 windowBgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImVec4 headerColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    ImVec4 unityProjectColor = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
    ImVec4 unrealProjectColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    ImVec4 buttonColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 buttonHoverColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    ImVec4 buttonActiveColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Layout
    float windowPadding = 10.0f;
    float itemSpacing = 8.0f;
    float columnWidth = 0.5f;
    float projectListHeight = 400.0f;
    bool showProjectType = true;
    bool showProjectPath = false;
    bool useCompactMode = false;

    // Window
    bool alwaysOnTop = false;
    bool rememberWindowPosition = true;
    bool rememberWindowSize = true;
    ImVec2 windowSize = ImVec2(1280, 720);
    ImVec2 windowPosition = ImVec2(0, 0);

    // Behavior
    bool autoScanOnStart = true;
    bool showHiddenFiles = false;
    bool sortProjectsByName = true;
    bool groupByType = true;
    int scanDepth = 5;
    bool showScanProgress = true;
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

void SaveSettings(const UISettings& settings) {
    std::string configPath = GetConfigPath();
    std::ofstream file(configPath + ".settings");
    if (file.is_open()) {
        // Save all settings
        file << "windowBgColor " << settings.windowBgColor.x << " " << settings.windowBgColor.y << " " << settings.windowBgColor.z << " " << settings.windowBgColor.w << "\n";
        file << "headerColor " << settings.headerColor.x << " " << settings.headerColor.y << " " << settings.headerColor.z << " " << settings.headerColor.w << "\n";
        file << "unityProjectColor " << settings.unityProjectColor.x << " " << settings.unityProjectColor.y << " " << settings.unityProjectColor.z << " " << settings.unityProjectColor.w << "\n";
        file << "unrealProjectColor " << settings.unrealProjectColor.x << " " << settings.unrealProjectColor.y << " " << settings.unrealProjectColor.z << " " << settings.unrealProjectColor.w << "\n";
        file << "buttonColor " << settings.buttonColor.x << " " << settings.buttonColor.y << " " << settings.buttonColor.z << " " << settings.buttonColor.w << "\n";
        file << "buttonHoverColor " << settings.buttonHoverColor.x << " " << settings.buttonHoverColor.y << " " << settings.buttonHoverColor.z << " " << settings.buttonHoverColor.w << "\n";
        file << "buttonActiveColor " << settings.buttonActiveColor.x << " " << settings.buttonActiveColor.y << " " << settings.buttonActiveColor.z << " " << settings.buttonActiveColor.w << "\n";
        file << "textColor " << settings.textColor.x << " " << settings.textColor.y << " " << settings.textColor.z << " " << settings.textColor.w << "\n";
        file << "windowPadding " << settings.windowPadding << "\n";
        file << "itemSpacing " << settings.itemSpacing << "\n";
        file << "columnWidth " << settings.columnWidth << "\n";
        file << "projectListHeight " << settings.projectListHeight << "\n";
        file << "showProjectType " << settings.showProjectType << "\n";
        file << "showProjectPath " << settings.showProjectPath << "\n";
        file << "useCompactMode " << settings.useCompactMode << "\n";
        file << "alwaysOnTop " << settings.alwaysOnTop << "\n";
        file << "rememberWindowPosition " << settings.rememberWindowPosition << "\n";
        file << "rememberWindowSize " << settings.rememberWindowSize << "\n";
        file << "windowSize " << settings.windowSize.x << " " << settings.windowSize.y << "\n";
        file << "windowPosition " << settings.windowPosition.x << " " << settings.windowPosition.y << "\n";
        file << "autoScanOnStart " << settings.autoScanOnStart << "\n";
        file << "showHiddenFiles " << settings.showHiddenFiles << "\n";
        file << "sortProjectsByName " << settings.sortProjectsByName << "\n";
        file << "groupByType " << settings.groupByType << "\n";
        file << "scanDepth " << settings.scanDepth << "\n";
        file << "showScanProgress " << settings.showScanProgress << "\n";
    }
}

UISettings LoadSettings() {
    UISettings settings;
    std::string configPath = GetConfigPath();
    std::ifstream file(configPath + ".settings");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key;
            iss >> key;
            
            if (key == "windowBgColor") iss >> settings.windowBgColor.x >> settings.windowBgColor.y >> settings.windowBgColor.z >> settings.windowBgColor.w;
            else if (key == "headerColor") iss >> settings.headerColor.x >> settings.headerColor.y >> settings.headerColor.z >> settings.headerColor.w;
            else if (key == "unityProjectColor") iss >> settings.unityProjectColor.x >> settings.unityProjectColor.y >> settings.unityProjectColor.z >> settings.unityProjectColor.w;
            else if (key == "unrealProjectColor") iss >> settings.unrealProjectColor.x >> settings.unrealProjectColor.y >> settings.unrealProjectColor.z >> settings.unrealProjectColor.w;
            else if (key == "buttonColor") iss >> settings.buttonColor.x >> settings.buttonColor.y >> settings.buttonColor.z >> settings.buttonColor.w;
            else if (key == "buttonHoverColor") iss >> settings.buttonHoverColor.x >> settings.buttonHoverColor.y >> settings.buttonHoverColor.z >> settings.buttonHoverColor.w;
            else if (key == "buttonActiveColor") iss >> settings.buttonActiveColor.x >> settings.buttonActiveColor.y >> settings.buttonActiveColor.z >> settings.buttonActiveColor.w;
            else if (key == "textColor") iss >> settings.textColor.x >> settings.textColor.y >> settings.textColor.z >> settings.textColor.w;
            else if (key == "windowPadding") iss >> settings.windowPadding;
            else if (key == "itemSpacing") iss >> settings.itemSpacing;
            else if (key == "columnWidth") iss >> settings.columnWidth;
            else if (key == "projectListHeight") iss >> settings.projectListHeight;
            else if (key == "showProjectType") iss >> settings.showProjectType;
            else if (key == "showProjectPath") iss >> settings.showProjectPath;
            else if (key == "useCompactMode") iss >> settings.useCompactMode;
            else if (key == "alwaysOnTop") iss >> settings.alwaysOnTop;
            else if (key == "rememberWindowPosition") iss >> settings.rememberWindowPosition;
            else if (key == "rememberWindowSize") iss >> settings.rememberWindowSize;
            else if (key == "windowSize") iss >> settings.windowSize.x >> settings.windowSize.y;
            else if (key == "windowPosition") iss >> settings.windowPosition.x >> settings.windowPosition.y;
            else if (key == "autoScanOnStart") iss >> settings.autoScanOnStart;
            else if (key == "showHiddenFiles") iss >> settings.showHiddenFiles;
            else if (key == "sortProjectsByName") iss >> settings.sortProjectsByName;
            else if (key == "groupByType") iss >> settings.groupByType;
            else if (key == "scanDepth") iss >> settings.scanDepth;
            else if (key == "showScanProgress") iss >> settings.showScanProgress;
        }
    }
    return settings;
}

void ApplySettings(const UISettings& settings) {
    ImGuiStyle& style = ImGui::GetStyle();
    // Modern style: rounded corners, accent color, soft background, larger font
    ImVec4 accent = ImVec4(0.20f, 0.55f, 0.90f, 1.00f); // blue accent
    ImVec4 bg = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    ImVec4 panel = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    ImVec4 text = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    ImVec4 border = ImVec4(0.22f, 0.23f, 0.29f, 1.00f);
    ImVec4 button = accent;
    ImVec4 buttonHover = ImVec4(0.25f, 0.60f, 1.00f, 1.00f);
    ImVec4 buttonActive = ImVec4(0.18f, 0.48f, 0.80f, 1.00f);

    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 18.0f;
    style.GrabMinSize = 14.0f;
    style.TabBarBorderSize = 1.0f;
    style.TabBarOverlineSize = 2.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_ChildBg] = panel;
    colors[ImGuiCol_PopupBg] = panel;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0,0,0,0);
    colors[ImGuiCol_FrameBg] = panel;
    colors[ImGuiCol_FrameBgHovered] = buttonHover;
    colors[ImGuiCol_FrameBgActive] = buttonActive;
    colors[ImGuiCol_TitleBg] = panel;
    colors[ImGuiCol_TitleBgActive] = accent;
    colors[ImGuiCol_TitleBgCollapsed] = panel;
    colors[ImGuiCol_MenuBarBg] = panel;
    colors[ImGuiCol_ScrollbarBg] = panel;
    colors[ImGuiCol_ScrollbarGrab] = accent;
    colors[ImGuiCol_ScrollbarGrabHovered] = buttonHover;
    colors[ImGuiCol_ScrollbarGrabActive] = buttonActive;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = buttonActive;
    colors[ImGuiCol_Button] = button;
    colors[ImGuiCol_ButtonHovered] = buttonHover;
    colors[ImGuiCol_ButtonActive] = buttonActive;
    colors[ImGuiCol_Header] = accent;
    colors[ImGuiCol_HeaderHovered] = buttonHover;
    colors[ImGuiCol_HeaderActive] = buttonActive;
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = buttonHover;
    colors[ImGuiCol_SeparatorActive] = buttonActive;
    colors[ImGuiCol_ResizeGrip] = accent;
    colors[ImGuiCol_ResizeGripHovered] = buttonHover;
    colors[ImGuiCol_ResizeGripActive] = buttonActive;
    colors[ImGuiCol_Tab] = panel;
    colors[ImGuiCol_TabHovered] = buttonHover;
    colors[ImGuiCol_TabActive] = accent;
    colors[ImGuiCol_TabUnfocused] = panel;
    colors[ImGuiCol_TabUnfocusedActive] = accent;
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = ImVec4(text.x, text.y, text.z, 0.5f);
    colors[ImGuiCol_TextSelectedBg] = accent;
    colors[ImGuiCol_DockingPreview] = accent;
    colors[ImGuiCol_DockingEmptyBg] = bg;

    // Apply window settings
    if (settings.alwaysOnTop) {
        GLFWwindow* window = glfwGetCurrentContext();
        if (window) {
            glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
        }
    }
}

void ShowSettingsWindow(UISettings& settings, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Settings", p_open)) {
        ImGui::End();
        return;
    }

    // Store original settings for comparison
    UISettings originalSettings = settings;

    if (ImGui::BeginTabBar("SettingsTabs")) {
        if (ImGui::BeginTabItem("Colors")) {
            ImGui::ColorEdit4("Window Background", (float*)&settings.windowBgColor);
            ImGui::ColorEdit4("Header", (float*)&settings.headerColor);
            ImGui::ColorEdit4("Unity Projects", (float*)&settings.unityProjectColor);
            ImGui::ColorEdit4("Unreal Projects", (float*)&settings.unrealProjectColor);
            ImGui::ColorEdit4("Buttons", (float*)&settings.buttonColor);
            ImGui::ColorEdit4("Button Hover", (float*)&settings.buttonHoverColor);
            ImGui::ColorEdit4("Button Active", (float*)&settings.buttonActiveColor);
            ImGui::ColorEdit4("Text", (float*)&settings.textColor);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Layout")) {
            ImGui::SliderFloat("Window Padding", &settings.windowPadding, 0.0f, 50.0f);
            ImGui::SliderFloat("Item Spacing", &settings.itemSpacing, 0.0f, 50.0f);
            ImGui::SliderFloat("Column Width", &settings.columnWidth, 0.1f, 0.9f);
            ImGui::SliderFloat("Project List Height", &settings.projectListHeight, 100.0f, 800.0f);
            ImGui::Checkbox("Show Project Type", &settings.showProjectType);
            ImGui::Checkbox("Show Project Path", &settings.showProjectPath);
            ImGui::Checkbox("Use Compact Mode", &settings.useCompactMode);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Window")) {
            ImGui::Checkbox("Always On Top", &settings.alwaysOnTop);
            ImGui::Checkbox("Remember Window Position", &settings.rememberWindowPosition);
            ImGui::Checkbox("Remember Window Size", &settings.rememberWindowSize);
            ImGui::InputFloat2("Window Size", (float*)&settings.windowSize);
            ImGui::InputFloat2("Window Position", (float*)&settings.windowPosition);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Behavior")) {
            ImGui::Checkbox("Auto Scan On Start", &settings.autoScanOnStart);
            ImGui::Checkbox("Show Hidden Files", &settings.showHiddenFiles);
            ImGui::Checkbox("Sort Projects By Name", &settings.sortProjectsByName);
            ImGui::Checkbox("Group By Type", &settings.groupByType);
            ImGui::SliderInt("Scan Depth", &settings.scanDepth, 1, 10);
            ImGui::Checkbox("Show Scan Progress", &settings.showScanProgress);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Apply")) {
        ApplySettings(settings);
        SaveSettings(settings);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults")) {
        settings = UISettings();
        ApplySettings(settings);
        SaveSettings(settings);
    }

    ImGui::End();
}

void ImGuiCustomTitleBar(GLFWwindow* window, bool* p_open) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 36));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::Begin("##CustomTitleBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec2 barMin = ImGui::GetWindowPos();
    ImVec2 barMax = ImVec2(barMin.x + ImGui::GetWindowWidth(), barMin.y + ImGui::GetWindowHeight());
    float btnSize = 24.0f;
    float btnPad = 8.0f;
    ImVec2 dragMin = barMin;
    ImVec2 dragMax = ImVec2(barMax.x - btnPad - btnSize * 2 - 8, barMax.y);
    auto inDragRect = [&](float x, float y) {
        return x >= dragMin.x && x < dragMax.x && y >= dragMin.y && y < dragMax.y;
    };

    // App title
    ImGui::SetCursorPosX(16);
    ImGui::SetCursorPosY(8);
    ImGui::PushFont(ImGui::GetFont());
    ImGui::Text("Project Navigator");
    ImGui::PopFont();

    // Minimize and Close buttons (right side)
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - btnPad - btnSize * 2, 6));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f,0.2f,0.2f,0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f,0.3f,0.3f,0.7f));
    if (ImGui::Button("\xef\x81\xb2##minimize", ImVec2(btnSize, btnSize))) {
#ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(window);
        ShowWindow(hwnd, SW_MINIMIZE);
#endif
    }
    bool hoveringButtons = ImGui::IsItemHovered();
    ImGui::SameLine(0, 4);
    ImGui::SetCursorPosY(6);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f,0.2f,0.2f,1.0f));
    if (ImGui::Button("\xef\x80\x8d##close", ImVec2(btnSize, btnSize))) {
        if (p_open) *p_open = false;
#ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(window);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
#endif
    }
    hoveringButtons = hoveringButtons || ImGui::IsItemHovered();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleVar(3);

#ifdef _WIN32
    ImGuiIO& io = ImGui::GetIO();
    HWND hwnd = glfwGetWin32Window(window);
    POINT mouse;
    GetCursorPos(&mouse);
    if (!hoveringButtons && io.MouseClicked[0] && inDragRect(mouse.x, mouse.y)) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        g_clickOffset.x = mouse.x - rect.left;
        g_clickOffset.y = mouse.y - rect.top;
        g_draggingWindow = true;
    }
    if (g_draggingWindow && io.MouseDown[0]) {
        GetCursorPos(&mouse);
        SetWindowPos(hwnd, nullptr, mouse.x - (int)g_clickOffset.x, mouse.y - (int)g_clickOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    if (g_draggingWindow && !io.MouseDown[0]) {
        g_draggingWindow = false;
    }
#endif
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
    glfwInit();
#ifdef _WIN32
    // Remove the GLFW_DECORATED hint to allow window resizing
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Project Navigator", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Center the window on screen
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - winWidth) / 2;
    int y = (screenHeight - winHeight) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
    static bool windowOpen = true;

    // Perform initial scan
    try {
        projects = ScanForProjects(dirBuffer);
        scanError.clear();
        scanned = true;
    } catch (const std::exception& e) {
        scanError = e.what();
        scanned = false;
    }

    while (!glfwWindowShouldClose(window) && windowOpen) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Custom title bar
        ImGuiCustomTitleBar(window, &windowOpen);

        // Modern dockspace layout
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // Main content window
        ImGui::Begin("ProjectNavigatorMain", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Enter the root directory to scan for Unity and Unreal projects:");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120);
        ImGui::InputText("##Root Directory", dirBuffer, sizeof(dirBuffer));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Scan for Projects", ImVec2(120, 0))) {
            try {
                projects = ScanForProjects(dirBuffer);
                scanError.clear();
                scanned = true;
                SaveLastDirectory(dirBuffer);
            } catch (const std::exception& e) {
                scanError = e.what();
                scanned = false;
            }
        }
        if (!scanError.empty()) {
            ImGui::TextColored(ImVec4(1,0,0,1), "Error: %s", scanError.c_str());
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Responsive panels for Unity and Unreal projects
        float panelWidth = (ImGui::GetContentRegionAvail().x - 24) * 0.5f;
        ImGui::BeginChild("UnityPanel", ImVec2(panelWidth, 0), true, ImGuiWindowFlags_None);
        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.8f, 1.0f), "Unity Projects");
        ImGui::Separator();
        bool hasUnityProjects = false;
        for (const auto& proj : projects) {
            if (proj.type == "Unity") {
                hasUnityProjects = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
                if (ImGui::Selectable(proj.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(panelWidth - 80, 0))) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
#ifdef _WIN32
                        std::string cmd = "explorer \"" + proj.path + "\"";
                        system(cmd.c_str());
#endif
                    }
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
                std::string btnId = "Open##Unity" + proj.name;
                if (ImGui::Button(btnId.c_str(), ImVec2(60, 0))) {
#ifdef _WIN32
                    std::string cmd = "explorer \"" + proj.path + "\"";
                    system(cmd.c_str());
#endif
                }
            }
        }
        if (!hasUnityProjects) {
            ImGui::TextDisabled("No Unity projects found");
        }
        ImGui::EndChild();
        ImGui::SameLine(0, 24);
        ImGui::BeginChild("UnrealPanel", ImVec2(panelWidth, 0), true, ImGuiWindowFlags_None);
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Unreal Projects");
        ImGui::Separator();
        bool hasUnrealProjects = false;
        for (const auto& proj : projects) {
            if (proj.type == "Unreal") {
                hasUnrealProjects = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Selectable(proj.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(panelWidth - 80, 0))) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
#ifdef _WIN32
                        std::string cmd = "explorer \"" + proj.path + "\"";
                        system(cmd.c_str());
#endif
                    }
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
                std::string btnId = "Open##Unreal" + proj.name;
                if (ImGui::Button(btnId.c_str(), ImVec2(60, 0))) {
#ifdef _WIN32
                    std::string cmd = "explorer \"" + proj.path + "\"";
                    system(cmd.c_str());
#endif
                }
            }
        }
        if (!hasUnrealProjects) {
            ImGui::TextDisabled("No Unreal projects found");
        }
        ImGui::EndChild();

        ImGui::End(); // ProjectNavigatorMain

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
