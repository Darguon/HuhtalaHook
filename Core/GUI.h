#pragma once
#include "..\Core\Config.h"
#include "..\Core\Render.h"
#include "..\Core\GitHubImageLoader.h"
#include "..\Features\Aimbot.h"
#include "..\Features\Misc.h"
#include "..\Features\TriggerBot.h"
#include "..\Config\ConfigMenu.h"
#include "..\Config\ConfigSaver.h"

#include "..\Resources\Language.hpp"
#include "..\Resources\Images.hpp"  // Keep for fallback
#include "../Helpers/KeyManager.h"

// Global image resources
ID3D11ShaderResourceView* Logo = NULL;
ID3D11ShaderResourceView* MenuButton1 = NULL;
ID3D11ShaderResourceView* MenuButton2 = NULL;
ID3D11ShaderResourceView* MenuButton3 = NULL;
ID3D11ShaderResourceView* MenuButton4 = NULL;
ID3D11ShaderResourceView* MenuButton1Pressed = NULL;
ID3D11ShaderResourceView* MenuButton2Pressed = NULL;
ID3D11ShaderResourceView* MenuButton3Pressed = NULL;
ID3D11ShaderResourceView* MenuButton4Pressed = NULL;
ID3D11ShaderResourceView* HitboxImage = NULL;

// Button states
bool Button1Pressed = true;
bool Button2Pressed = false;
bool Button3Pressed = false;
bool Button4Pressed = false;

// Add these new animation variables
float tabAnimProgress = 1.0f;         // Start at 1.0f since Visual tab is active by default
int lastActiveTab = 0;                // Track the previous active tab
float tabTransitionSpeed = 4.0f;      // Control animation speed (higher = faster)
bool isAnimating = false;

// Image dimensions
int LogoW = 0, LogoH = 0;
int buttonW = 0;
int buttonH = 0;
int hitboxW = 0, hitboxH = 0;

// Hitbox checkboxes
bool checkbox1 = true;
bool checkbox2 = false;
bool checkbox3 = false;
bool checkbox4 = false;
bool checkbox5 = false;

// Forward declarations - needs to be at the top
void DrawCardHeader(const char* title, ImVec2 pos, ImDrawList* drawList);
void LoadImages();

namespace ConfigMenu
{
    // Instead of implementing the config menu functionality directly here,
    // we'll just declare the function which is implemented elsewhere
    void RenderCFGmenu();
}

namespace GUI
{
    // Function declarations - must be declared before they are used!
    void DrawGui();
    void SwitchButton(const char* str_id, bool* v);
    void AlignRight(float ContentWidth);
    void PutSwitch(const char* string, float CursorX, float ContentWidth, bool* v, bool ColorEditor = false, const char* lable = NULL, float col[4] = NULL, const char* Tip = NULL);
    void PutColorEditor(const char* text, const char* lable, float CursorX, float ContentWidth, float col[4], const char* Tip = NULL);
    void PutSliderFloat(const char* string, float CursorX, float* v, const void* p_min, const void* p_max, const char* format);
    void PutSliderInt(const char* string, float CursorX, int* v, const void* p_min, const void* p_max, const char* format);
    void InitHitboxList();
    void addHitbox(int BoneIndex);
    void removeHitbox(int BoneIndex);
    void LoadDefaultConfig();
    void ApplyModernStyle();
}

// Global function to load images
void LoadImages()
{
    if (Logo == NULL)
    {
        // Try to load images from memory
        Gui.LoadTextureFromMemory(Images::Logo, sizeof Images::Logo, &Logo, &LogoW, &LogoH);
        Gui.LoadTextureFromMemory(Images::VisualButton, sizeof Images::VisualButton, &MenuButton1, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::AimbotButton, sizeof Images::AimbotButton, &MenuButton2, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::MiscButton, sizeof Images::MiscButton, &MenuButton3, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::ConfigButton, sizeof Images::ConfigButton, &MenuButton4, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::PreviewImg, sizeof Images::PreviewImg, &HitboxImage, &hitboxW, &hitboxH);
        Gui.LoadTextureFromMemory(Images::VisualButtonPressed, sizeof Images::VisualButtonPressed, &MenuButton1Pressed, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::AimbotButtonPressed, sizeof Images::AimbotButtonPressed, &MenuButton2Pressed, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::MiscButtonPressed, sizeof Images::MiscButtonPressed, &MenuButton3Pressed, &buttonW, &buttonH);
        Gui.LoadTextureFromMemory(Images::ConfigButtonPressed, sizeof Images::ConfigButtonPressed, &MenuButton4Pressed, &buttonW, &buttonH);

        // Ensure proper positioning of windows
        MenuConfig::MarkWinPos = ImVec2(ImGui::GetIO().DisplaySize.x - 300.0f, 100.f);
        MenuConfig::SpecWinPos = ImVec2(10.0f, ImGui::GetIO().DisplaySize.y / 2 - 200);
        MenuConfig::BombWinPos = ImVec2((ImGui::GetIO().DisplaySize.x - 200.0f) / 2.0f, 80.0f);
    }
}

// This is a utility function that needs to be outside the namespace
void DrawCardHeader(const char* title, ImVec2 pos, ImDrawList* drawList)
{
    ImVec2 textSize = ImGui::CalcTextSize(title);
    float headerWidth = textSize.x + 10.0f;

    // Draw header text with shadow
    drawList->AddText(
        ImVec2(pos.x + 1, pos.y + 1),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.5f)),
        title
    );

    // Draw header text
    drawList->AddText(
        pos,
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.28f, 1.00f)),
        title
    );

    // Draw underline
    drawList->AddRectFilled(
        ImVec2(pos.x, pos.y + textSize.y + 2),
        ImVec2(pos.x + headerWidth, pos.y + textSize.y + 3),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.28f, 0.80f))
    );

    // Draw fade-out extension of underline
    for (int i = 0; i < 50; i++) {
        float alpha = 0.4f * (1.0f - (float)i / 50.0f);
        drawList->AddRectFilled(
            ImVec2(pos.x + headerWidth + i * 2, pos.y + textSize.y + 2),
            ImVec2(pos.x + headerWidth + (i + 1) * 2, pos.y + textSize.y + 3),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.28f, alpha))
        );
    }

    // Add extra spacing
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textSize.y + 8);
}

// Implementation of the GUI namespace functions
namespace GUI
{
    void LoadDefaultConfig()
    {
        if (!MenuConfig::defaultConfig)
            return;

        MyConfigSaver::LoadConfig("default.cfg");

        MenuConfig::defaultConfig = false;
    }

    // Initialize the hitbox list
    void InitHitboxList()
    {
        if (LegitBotConfig::HitboxUpdated)
            return;
        auto HitboxList = AimControl::HitboxList;

        auto it = std::find(HitboxList.begin(), HitboxList.end(), BONEINDEX::head);
        if (it != HitboxList.end())
            checkbox1 = true;

        it = std::find(HitboxList.begin(), HitboxList.end(), BONEINDEX::neck_0);
        if (it != HitboxList.end())
            checkbox2 = true;

        it = std::find(HitboxList.begin(), HitboxList.end(), BONEINDEX::spine_1);
        if (it != HitboxList.end())
            checkbox3 = true;

        it = std::find(HitboxList.begin(), HitboxList.end(), BONEINDEX::spine_2);
        if (it != HitboxList.end())
            checkbox4 = true;

        it = std::find(HitboxList.begin(), HitboxList.end(), BONEINDEX::pelvis);
        if (it != HitboxList.end())
            checkbox5 = true;

        LegitBotConfig::HitboxUpdated = true;
    }

    void addHitbox(int BoneIndex)
    {
        AimControl::HitboxList.push_back(BoneIndex);
    }

    void removeHitbox(int BoneIndex)
    {
        for (auto it = AimControl::HitboxList.begin(); it != AimControl::HitboxList.end(); ++it) {
            if (*it == BoneIndex) {
                AimControl::HitboxList.erase(it);
                break;
            }
        }
    }

    // UI Component functions
    void AlignRight(float ContentWidth)
    {
        float ColumnContentWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x;
        float checkboxPosX = ImGui::GetColumnOffset() + ColumnContentWidth - ContentWidth;
        ImGui::SetCursorPosX(checkboxPosX);
    }

    void PutSwitch(const char* string, float CursorX, float ContentWidth, bool* v, bool ColorEditor, const char* lable, float col[4], const char* Tip)
    {
        ImGui::PushID(string);
        float CurrentCursorX = ImGui::GetCursorPosX();
        float CurrentCursorY = ImGui::GetCursorPosY();
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::TextDisabled(string);
        if (Tip && ImGui::IsItemHovered())
            ImGui::SetTooltip(Tip);
        ImGui::SameLine();
        ImGui::SetCursorPosY(CurrentCursorY - 2);
        if (ColorEditor) {
            AlignRight(ContentWidth + ImGui::GetFrameHeight() + 7);
            ImGui::ColorEdit4(lable, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
            ImGui::SameLine();
        }
        else {
            AlignRight(ContentWidth);
        }

        SwitchButton(string, v);
        ImGui::PopID();
    }

    void PutColorEditor(const char* text, const char* lable, float CursorX, float ContentWidth, float col[4], const char* Tip)
    {
        ImGui::PushID(text);
        float CurrentCursorX = ImGui::GetCursorPosX();
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::TextDisabled(text);
        if (Tip && ImGui::IsItemHovered())
            ImGui::SetTooltip(Tip);
        ImGui::SameLine();
        AlignRight(ContentWidth + ImGui::GetFrameHeight() + 8);
        ImGui::ColorEdit4(lable, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
        ImGui::PopID();
    }

    void PutSliderFloat(const char* string, float CursorX, float* v, const void* p_min, const void* p_max, const char* format)
    {
        // if there is no fucking ID, all the sliders would be fucking forced to sync when you click on one of them ;3
        ImGui::PushID(string);
        float CurrentCursorX = ImGui::GetCursorPosX();
        float SliderWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - CursorX - 15;
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::TextDisabled(string);
        ImGui::SameLine();
        ImGui::TextDisabled(format, *v);
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::SetNextItemWidth(SliderWidth);
        Gui.SliderScalarEx2("", ImGuiDataType_Float, v, p_min, p_max, "", ImGuiSliderFlags_None);
        ImGui::PopID();
    }

    void PutSliderInt(const char* string, float CursorX, int* v, const void* p_min, const void* p_max, const char* format)
    {
        ImGui::PushID(string);
        float CurrentCursorX = ImGui::GetCursorPosX();
        float SliderWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - CursorX - 15;
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::TextDisabled(string);
        ImGui::SameLine();
        ImGui::TextDisabled(format, *v);
        ImGui::SetCursorPosX(CurrentCursorX + CursorX);
        ImGui::SetNextItemWidth(SliderWidth);
        Gui.SliderScalarEx2("", ImGuiDataType_Float, v, p_min, p_max, "", ImGuiSliderFlags_None);
        ImGui::PopID();
    }

    // Apply modern styles
    void ApplyModernStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Main colors - dark black theme with midnight blue accent
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.10f, 0.98f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.80f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.10f, 0.94f);

        // Headers - midnight blue
        colors[ImGuiCol_Header] = ImVec4(0.10f, 0.12f, 0.28f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.14f, 0.32f, 0.70f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.16f, 0.36f, 0.80f);

        // Buttons - midnight blue
        colors[ImGuiCol_Button] = ImVec4(0.10f, 0.12f, 0.28f, 0.60f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.14f, 0.32f, 0.70f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.16f, 0.36f, 0.80f);

        // Frame BG (checkboxes, radio buttons, etc.)
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.14f, 0.80f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.14f, 0.32f, 0.30f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.14f, 0.16f, 0.36f, 0.50f);

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.14f, 0.80f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.12f, 0.14f, 0.32f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.14f, 0.16f, 0.36f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.12f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.12f, 0.28f, 1.00f);

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.28f, 0.90f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.10f, 0.75f);

        // Borders, separators
        colors[ImGuiCol_Border] = ImVec4(0.10f, 0.12f, 0.28f, 0.40f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.12f, 0.28f, 0.40f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.12f, 0.14f, 0.32f, 0.60f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.16f, 0.36f, 0.80f);

        // Sliders, progress bars
        colors[ImGuiCol_SliderGrab] = ImVec4(0.10f, 0.12f, 0.28f, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.12f, 0.14f, 0.32f, 1.00f);

        // Modern Style - increased roundness and spacing
        style.WindowRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        // Borders
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        // Item spacing for better layout
        style.ItemSpacing = ImVec2(10.0f, 10.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 22.0f;
        style.ScrollbarSize = 10.0f;
        style.GrabMinSize = 8.0f;
    }

    // This is the main entry point for the GUI
    void DrawGui()
    {
        // Apply modern style
        ApplyModernStyle();

        // Load images if needed (calling the global function)
        ::LoadImages();

        // Define colors for use in the UI
        ImVec4 BorderColorVec4 = ImVec4(0.10f, 0.12f, 0.28f, 0.80f);
        ImVec4 HoverColorVec4 = ImVec4(0.12f, 0.14f, 0.32f, 0.80f);
        ImVec4 ActiveColorVec4 = ImVec4(0.14f, 0.16f, 0.36f, 1.00f);
        ImU32 BorderColorU32 = ImGui::ColorConvertFloat4ToU32(BorderColorVec4);
        ImU32 ShadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.25f));

        // Main window setup
        char TempText[256];
        ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
        ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - MenuConfig::WCS.MainWinSize.x) / 2.0f, (ImGui::GetIO().DisplaySize.y - MenuConfig::WCS.MainWinSize.y) / 2.0f }, ImGuiCond_Once);
        ImGui::SetNextWindowSize(MenuConfig::WCS.MainWinSize);
        ImGui::GetStyle().WindowRounding = 8.0f;
        ImGui::Begin("HuhtalaHook", nullptr, Flags);
        {
            // Get window dimensions for layout
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Modern layout with floating sidebar and content card
            float sidebarWidth = 75.0f;
            float contentAreaWidth = windowSize.x - sidebarWidth - 15.0f;

            // Draw window shadow effect (simple rect with dark color)
            drawList->AddRect(
                ImVec2(windowPos.x - 2, windowPos.y - 2),
                ImVec2(windowPos.x + windowSize.x + 2, windowPos.y + windowSize.y + 2),
                ShadowColor, 8.0f, 0, 2.0f
            );

            // Draw sidebar background
            ImVec2 sidebarMin = windowPos;
            ImVec2 sidebarMax = ImVec2(windowPos.x + sidebarWidth, windowPos.y + windowSize.y);

            // Dark sidebar background
            drawList->AddRectFilled(
                sidebarMin,
                sidebarMax,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.12f, 1.00f)),
                8.0f, ImDrawCornerFlags_Left
            );

            // Subtle sidebar separator
            drawList->AddLine(
                ImVec2(windowPos.x + sidebarWidth - 1, windowPos.y + 5),
                ImVec2(windowPos.x + sidebarWidth - 1, windowPos.y + windowSize.y - 5),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.28f, 0.30f)),
                1.0f
            );

            // Title bar with logo - centered in sidebar
            ImTextureID LogoID = (void*)Logo;
            ImVec2 LogoSize = ImVec2(LogoW * 0.8f, LogoH * 0.8f); // Slightly smaller

            // Show logo
            ImGui::SetCursorPos(ImVec2((sidebarWidth - LogoSize.x) / 2.0f, 15.0f));
            ImGui::Image(LogoID, LogoSize);
            if (ImGui::IsItemClicked()) {
                Gui.OpenWebpage("");
            }

            // Navigation buttons - vertical layout in sidebar with icons
            float buttonY = 15.0f + LogoSize.y + 25.0f;
            float buttonSpacing = 15.0f;
            float buttonSize = 50.0f;  // Square buttons for modern look

            // Visual button
            ImGui::SetCursorPos(ImVec2((sidebarWidth - buttonSize) / 2.0f, buttonY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            if (Button1Pressed)
                ImGui::PushStyleColor(ImGuiCol_Button, ActiveColorVec4);
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 0.80f));

            if (ImGui::Button("V", ImVec2(buttonSize, buttonSize))) {
                MenuConfig::WCS.MenuPage = 0;
                Button1Pressed = true;
                Button2Pressed = false;
                Button3Pressed = false;
                Button4Pressed = false;
                isAnimating = true;
                tabAnimProgress = 0.0f;
                lastActiveTab = MenuConfig::WCS.MenuPage;
            }
            ImGui::PopStyleColor();

            buttonY += buttonSize + buttonSpacing;

            // Aimbot button
            ImGui::SetCursorPos(ImVec2((sidebarWidth - buttonSize) / 2.0f, buttonY));
            if (Button2Pressed)
                ImGui::PushStyleColor(ImGuiCol_Button, ActiveColorVec4);
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 0.80f));

            if (ImGui::Button("A", ImVec2(buttonSize, buttonSize))) {
                MenuConfig::WCS.MenuPage = 1;
                Button1Pressed = false;
                Button2Pressed = true;
                Button3Pressed = false;
                Button4Pressed = false;
                isAnimating = true;
                tabAnimProgress = 0.0f;
                lastActiveTab = MenuConfig::WCS.MenuPage;
            }
            ImGui::PopStyleColor();

            buttonY += buttonSize + buttonSpacing;

            // Misc button
            ImGui::SetCursorPos(ImVec2((sidebarWidth - buttonSize) / 2.0f, buttonY));
            if (Button3Pressed)
                ImGui::PushStyleColor(ImGuiCol_Button, ActiveColorVec4);
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 0.80f));

            if (ImGui::Button("M", ImVec2(buttonSize, buttonSize))) {
                MenuConfig::WCS.MenuPage = 2;
                Button1Pressed = false;
                Button2Pressed = false;
                Button3Pressed = true;
                Button4Pressed = false;
                isAnimating = true;
                tabAnimProgress = 0.0f;
                lastActiveTab = MenuConfig::WCS.MenuPage;
            }
            ImGui::PopStyleColor();

            buttonY += buttonSize + buttonSpacing;

            // Config button
            ImGui::SetCursorPos(ImVec2((sidebarWidth - buttonSize) / 2.0f, buttonY));
            if (Button4Pressed)
                ImGui::PushStyleColor(ImGuiCol_Button, ActiveColorVec4);
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.16f, 0.80f));

            if (ImGui::Button("C", ImVec2(buttonSize, buttonSize))) {
                MenuConfig::WCS.MenuPage = 3;
                Button1Pressed = false;
                Button2Pressed = false;
                Button3Pressed = false;
                Button4Pressed = true;
                isAnimating = true;
                tabAnimProgress = 0.0f;
                lastActiveTab = MenuConfig::WCS.MenuPage;
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            // Get current tab and handle animations
            int currentTab = MenuConfig::WCS.MenuPage;

            // Check if tab changed
            if (currentTab != lastActiveTab && !isAnimating) {
                isAnimating = true;
                tabAnimProgress = 0.0f;
                lastActiveTab = currentTab;
            }

            // Update animation progress - slightly faster for snappier feel
            if (isAnimating) {
                tabAnimProgress += ImGui::GetIO().DeltaTime * (tabTransitionSpeed * 1.2f);
                if (tabAnimProgress >= 1.0f) {
                    tabAnimProgress = 1.0f;
                    isAnimating = false;
                }
            }

            // Calculate smooth animation effects
            float slideDirection = lastActiveTab < currentTab ? 1.0f : -1.0f; // Right or left
            float slideOffset = 0.0f;
            float fadeAlpha = 1.0f;

            if (isAnimating) {
                // Simple easing
                float easedProgress = tabAnimProgress;

                // Apply slide effect
                slideOffset = (1.0f - easedProgress) * 40.0f * slideDirection;

                // Apply fade effect
                fadeAlpha = tabAnimProgress < 0.3f ? tabAnimProgress * 3.33f : 1.0f;
            }

            // Content area with modern card styling
            ImVec2 contentAreaPos = ImVec2(windowPos.x + sidebarWidth + 10.0f + slideOffset, windowPos.y + 12.0f);
            ImVec2 contentAreaSize = ImVec2(contentAreaWidth - 5.0f, windowSize.y - 24.0f);

            // Draw content area background
            drawList->AddRectFilled(
                contentAreaPos,
                ImVec2(contentAreaPos.x + contentAreaSize.x, contentAreaPos.y + contentAreaSize.y),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.12f, 0.14f, 0.95f)),
                8.0f
            );

            // Subtle glow on border
            drawList->AddRect(
                contentAreaPos,
                ImVec2(contentAreaPos.x + contentAreaSize.x, contentAreaPos.y + contentAreaSize.y),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.28f, 0.30f)),
                8.0f, 0, 1.0f
            );

            // Set cursor to content area and apply fade effect
            ImGui::SetCursorPos(ImVec2(sidebarWidth + 20.0f + slideOffset, 20.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fadeAlpha);

            // Begin content area
            ImGui::BeginChild("ContentArea", ImVec2(contentAreaWidth - 25.0f, windowSize.y - 40.0f), false, ImGuiWindowFlags_NoScrollbar);
            {
                // Title with tab name - modern typography
                const char* tabNames[] = { "Visual Settings", "Aimbot Settings", "Miscellaneous", "Configuration" };

                // Draw title with accent underline
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font
                ImVec2 titlePos = ImGui::GetCursorScreenPos();
                float titleWidth = ImGui::CalcTextSize(tabNames[MenuConfig::WCS.MenuPage]).x;

                ImGui::Text("%s", tabNames[MenuConfig::WCS.MenuPage]);

                // Animated underline
                float underlineProgress = isAnimating ? tabAnimProgress : 1.0f;
                drawList->AddRectFilled(
                    ImVec2(titlePos.x, titlePos.y + 24),
                    ImVec2(titlePos.x + titleWidth * underlineProgress, titlePos.y + 26),
                    ImGui::ColorConvertFloat4ToU32(BorderColorVec4)
                );

                ImGui::PopFont();
                ImGui::Spacing();
                ImGui::Spacing();

                // Content based on selected tab
                if (MenuConfig::WCS.MenuPage == 0) {
                    // Visual Settings Tab
                    ImGui::Columns(2, nullptr, false);

                    // First section: ESP Settings
                    ::DrawCardHeader("ESP Settings", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    InitHitboxList();

                    float MinRounding = 0.f, MaxRouding = 5.f;
                    PutSwitch(Text::ESP::Toggle.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ESPenabled);
                    if (ESPConfig::ESPenabled)
                    {
                        PutSwitch(Text::ESP::Box.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowBoxESP, true, "###BoxCol", reinterpret_cast<float*>(&ESPConfig::BoxColor));
                        if (ESPConfig::ShowBoxESP)
                        {
                            PutSwitch(Text::ESP::Outline.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::OutLine);
                            ImGui::TextDisabled(Text::ESP::BoxType.c_str());
                            ImGui::SameLine();
                            AlignRight(160.f);
                            ImGui::SetNextItemWidth(160.f);
                            ImGui::Combo("###BoxType", &ESPConfig::BoxType, "Normal\0Corner\0");
                            PutSliderFloat(Text::ESP::BoxRounding.c_str(), 10.f, &ESPConfig::BoxRounding, &MinRounding, &MaxRouding, "%.1f");
                        }
                        PutSwitch(Text::ESP::FilledBox.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::FilledBox, true, "###FilledBoxCol", reinterpret_cast<float*>(&ESPConfig::FilledColor));
                        if (ESPConfig::FilledBox)
                            PutSwitch(Text::ESP::MultiColor.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::MultiColor, true, "###MultiCol", reinterpret_cast<float*>(&ESPConfig::FilledColor2));
                        PutSwitch(Text::ESP::HeadBox.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowHeadBox, true, "###HeadBoxCol", reinterpret_cast<float*>(&ESPConfig::HeadBoxColor));
                        PutSwitch(Text::ESP::Skeleton.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowBoneESP, true, "###BoneCol", reinterpret_cast<float*>(&ESPConfig::BoneColor));
                        PutSwitch(Text::ESP::SnapLine.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowLineToEnemy, true, "###LineCol", reinterpret_cast<float*>(&ESPConfig::LineToEnemyColor));
                        if (ESPConfig::ShowLineToEnemy)
                        {
                            ImGui::TextDisabled(Text::ESP::LinePosList.c_str());
                            ImGui::SameLine();
                            AlignRight(160.f);
                            ImGui::SetNextItemWidth(160.f);
                            ImGui::Combo("###LinePos", &ESPConfig::LinePos, "Top\0Center\0Bottom\0");
                        }
                        PutSwitch(Text::ESP::EyeRay.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowEyeRay, true, "###LineCol", reinterpret_cast<float*>(&ESPConfig::EyeRayColor));
                        PutSwitch(Text::ESP::HealthBar.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowHealthBar);
                        if (ESPConfig::ShowHealthBar)
                            PutSwitch(Text::ESP::HealthNum.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowHealthNum);
                        PutSwitch(Text::ESP::ShowArmorBar.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ArmorBar);
                        if (ESPConfig::ArmorBar)
                            PutSwitch(Text::ESP::ArmorNum.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowArmorNum);
                        PutSwitch(Text::ESP::Weapon.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowWeaponESP);
                        PutSwitch(Text::ESP::Ammo.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::AmmoBar);
                        PutSwitch(Text::ESP::Distance.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowDistance);
                        PutSwitch(Text::ESP::PlayerName.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowPlayerName);
                        PutSwitch(Text::ESP::ScopedESP.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowIsScoped);
                        PutSwitch(Text::ESP::VisCheck.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::VisibleCheck, true, "###VisibleCol", reinterpret_cast<float*>(&ESPConfig::VisibleColor));
                    }

                    ImGui::NextColumn();

                    // Second section: ESP Preview
                    ::DrawCardHeader("ESP Preview", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // Use existing preview rendering
                    ESP::RenderPreview({ ImGui::GetColumnWidth() - 20.f, ImGui::GetContentRegionAvail().y * 0.5f - 30.f });

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Columns(1);
                }
                else if (MenuConfig::WCS.MenuPage == 1) {
                    // Aimbot Settings Tab
                    ImGui::Columns(2, nullptr, false);

                    // First section: Aimbot
                    ::DrawCardHeader("Aimbot", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // Aimbot controls...
                    float FovMin = 0.f, FovMax = 30.f, MinFovMax = 1.f;
                    int BulletMin = 0, BulletMax = 5;
                    float SmoothMin = 1.f, SmoothMax = 15.f;

                    PutSwitch(Text::Aimbot::Enable.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::AimBot);
                    if (LegitBotConfig::AimBot)
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                        ImGui::TextDisabled(Text::Aimbot::HotKeyList.c_str());
                        ImGui::SameLine();
                        AlignRight(70.f);
                        if (ImGui::Button(Text::Aimbot::HotKey.c_str(), { 70.f, 25.f }))
                        {
                            std::thread([&]() {
                                KeyMgr::GetPressedKey(AimControl::HotKey, Text::Aimbot::HotKey);
                                }).detach();
                        }
                        PutSliderInt(Text::Aimbot::BulletSlider.c_str(), 10.f, &AimControl::AimBullet, &BulletMin, &BulletMax, "%d");
                        PutSwitch(Text::Aimbot::Toggle.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::AimToggleMode);
                        PutSwitch(Text::Aimbot::DrawFov.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::DrawFov, true, "###FOVcol", reinterpret_cast<float*>(&LegitBotConfig::FovCircleColor));
                        PutSwitch(Text::Aimbot::VisCheck.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::VisibleCheck);
                        PutSwitch(Text::Aimbot::OnlyAuto.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &AimControl::onlyAuto, false, NULL, NULL, Text::Aimbot::OnlyAutoTip.c_str());
                        PutSwitch(Text::Aimbot::IgnoreFlash.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &AimControl::IgnoreFlash);
                        PutSwitch(Text::Aimbot::ScopeOnly.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &AimControl::ScopeOnly);
                        PutSliderFloat(Text::Aimbot::FovSlider.c_str(), 10.f, &AimControl::AimFov, &AimControl::AimFovMin, &FovMax, "%.1f");
                        PutSliderFloat(Text::Aimbot::FovMinSlider.c_str(), 10.f, &AimControl::AimFovMin, &FovMin, &MinFovMax, "%.2f");
                        PutSliderFloat(Text::Aimbot::SmoothSlider.c_str(), 10.f, &AimControl::Smooth, &SmoothMin, &SmoothMax, "%.1f");
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                        ImGui::TextDisabled(Text::Aimbot::BoneList.c_str());

                        ImVec2 StartPos = ImGui::GetCursorScreenPos();
                        ImGui::Image((void*)HitboxImage, ImVec2(hitboxW, hitboxH));

                        ImGui::GetWindowDrawList()->AddLine(ImVec2(StartPos.x + 130, StartPos.y + 20), ImVec2(StartPos.x + 205, StartPos.y + 20), ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.8f); // Head
                        ImGui::SetCursorScreenPos(ImVec2(StartPos.x + 203, StartPos.y + 10));
                        if (ImGui::Checkbox("###Head", &checkbox1))
                        {
                            if (checkbox1) {
                                addHitbox(BONEINDEX::head);
                            }
                            else {
                                removeHitbox(BONEINDEX::head);
                            }
                        }
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(StartPos.x + 129, StartPos.y + 56), ImVec2(StartPos.x + 59, StartPos.y + 56), ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.8f); // Neck
                        ImGui::SetCursorScreenPos(ImVec2(StartPos.x + 39, StartPos.y + 45));
                        if (ImGui::Checkbox("###Neck", &checkbox2))
                        {
                            if (checkbox2) {
                                addHitbox(BONEINDEX::neck_0);
                            }
                            else {
                                removeHitbox(BONEINDEX::neck_0);
                            }
                        }
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(StartPos.x + 120, StartPos.y + 80), ImVec2(StartPos.x + 195, StartPos.y + 80), ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.8f);
                        ImGui::SetCursorScreenPos(ImVec2(StartPos.x + 193, StartPos.y + 70));
                        if (ImGui::Checkbox("###Chest", &checkbox3))
                        {
                            if (checkbox3) {
                                addHitbox(BONEINDEX::spine_1);
                            }
                            else {
                                removeHitbox(BONEINDEX::spine_1);
                            }
                        }
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(StartPos.x + 119, StartPos.y + 120), ImVec2(StartPos.x + 44, StartPos.y + 120), ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.8f);
                        ImGui::SetCursorScreenPos(ImVec2(StartPos.x + 24, StartPos.y + 109));
                        if (ImGui::Checkbox("###Stomache", &checkbox4))
                        {
                            if (checkbox4) {
                                addHitbox(BONEINDEX::spine_2);
                            }
                            else {
                                removeHitbox(BONEINDEX::spine_2);
                            }
                        }
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(StartPos.x + 119, StartPos.y + 150), ImVec2(StartPos.x + 195, StartPos.y + 150), ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.8f);
                        ImGui::SetCursorScreenPos(ImVec2(StartPos.x + 193, StartPos.y + 139));
                        if (ImGui::Checkbox("###Pelvis", &checkbox5))
                        {
                            if (checkbox4) {
                                addHitbox(BONEINDEX::pelvis);
                            }
                            else {
                                removeHitbox(BONEINDEX::pelvis);
                            }
                        }
                    }

                    ImGui::NextColumn();

                    // Second section: RCS
                    ::DrawCardHeader("Recoil Control", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // RCS controls...
                    float recoilMin = 0.f, recoilMax = 2.f;
                    int RCSBulletMin = 0, RCSBulletMax = 5;
                    PutSwitch(Text::RCS::Toggle.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::RCS);
                    if (LegitBotConfig::RCS)
                    {
                        PutSliderInt(Text::RCS::BulletSlider.c_str(), 5.f, &RCS::RCSBullet, &RCSBulletMin, &RCSBulletMax, "%d");
                        PutSliderFloat(Text::RCS::Yaw.c_str(), 5.f, &RCS::RCSScale.x, &recoilMin, &recoilMax, "%.2f");
                        PutSliderFloat(Text::RCS::Pitch.c_str(), 5.f, &RCS::RCSScale.y, &recoilMin, &recoilMax, "%.2f");
                        float scalex = (2.22 - RCS::RCSScale.x) * .5f;
                        float scaley = (2.12 - RCS::RCSScale.y) * .5f;//Simulate reasonable error values
                        ImVec2 BulletPos = ImGui::GetCursorScreenPos();

                        // Example Preview
                        ImVec2 BulletPos0, BulletPos1, BulletPos2, BulletPos3, BulletPos4, BulletPos5, BulletPos6, BulletPos7, BulletPos8, BulletPos9, BulletPos10, BulletPos11, BulletPos12, BulletPos13, BulletPos14, BulletPos15;
                        BulletPos.y += 123 * scaley;
                        BulletPos0.x = BulletPos.x + 125; BulletPos0.y = BulletPos.y + 5;
                        BulletPos1.x = BulletPos0.x - 3 * scalex; BulletPos1.y = BulletPos0.y - 5 * scaley;
                        BulletPos2.x = BulletPos1.x + 2 * scalex; BulletPos2.y = BulletPos1.y - 10 * scaley;
                        BulletPos3.x = BulletPos2.x + 4 * scalex; BulletPos3.y = BulletPos2.y - 11 * scaley;
                        BulletPos4.x = BulletPos3.x - 3 * scalex; BulletPos4.y = BulletPos3.y - 31 * scaley;
                        BulletPos5.x = BulletPos4.x - 1 * scalex; BulletPos5.y = BulletPos4.y - 20 * scaley;
                        BulletPos6.x = BulletPos5.x - 2 * scalex; BulletPos6.y = BulletPos5.y - 17 * scaley;
                        BulletPos7.x = BulletPos6.x - 15 * scalex; BulletPos7.y = BulletPos6.y - 9 * scaley;
                        BulletPos8.x = BulletPos7.x + 7 * scalex; BulletPos8.y = BulletPos7.y - 8 * scaley;
                        BulletPos9.x = BulletPos8.x + 33 * scalex; BulletPos9.y = BulletPos8.y + 2 * scaley;
                        BulletPos10.x = BulletPos9.x + 1 * scalex; BulletPos10.y = BulletPos9.y - 16 * scaley;
                        BulletPos11.x = BulletPos10.x - 9 * scalex; BulletPos11.y = BulletPos10.y + 20 * scaley;
                        BulletPos12.x = BulletPos11.x - 3 * scalex; BulletPos12.y = BulletPos11.y - 9 * scaley;
                        BulletPos13.x = BulletPos12.x + 15 * scalex; BulletPos13.y = BulletPos12.y - 5 * scaley;
                        BulletPos14.x = BulletPos13.x + 10 * scalex; BulletPos14.y = BulletPos13.y - 4 * scaley;

                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos0, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos1, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos2, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos3, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos4, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos5, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos6, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos7, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos8, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos9, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos10, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos11, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos12, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos13, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));
                        ImGui::GetWindowDrawList()->AddCircleFilled(BulletPos14, 4.f, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)));

                        ImGui::SetCursorScreenPos(ImVec2(BulletPos.x, BulletPos.y + 10));
                    }

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Third section: TriggerBot
                    ::DrawCardHeader("TriggerBot", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // Triggerbot controls...
                    int DelayMin = 0, DelayMax = 300;
                    int DurationMin = 0, DurationMax = 1000;

                    PutSwitch(Text::Trigger::Enable.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::TriggerBot);
                    if (LegitBotConfig::TriggerBot)
                    {
                        if (!LegitBotConfig::TriggerAlways)
                        {
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
                            ImGui::TextDisabled(Text::Trigger::HotKeyList.c_str());
                            ImGui::SameLine();
                            AlignRight(70.f);
                            if (ImGui::Button(Text::Trigger::HotKey.c_str(), { 70.f, 25.f }))
                            {
                                std::thread([&]() {
                                    KeyMgr::GetPressedKey(TriggerBot::HotKey, Text::Trigger::HotKey);
                                    }).detach();
                            }
                        }
                        PutSwitch(Text::Trigger::Toggle.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &LegitBotConfig::TriggerAlways);
                        PutSwitch(Text::Trigger::ScopeOnly.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &TriggerBot::ScopeOnly);
                        PutSwitch(Text::Trigger::IgnoreFlash.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &TriggerBot::IgnoreFlash);
                        PutSliderInt(Text::Trigger::DelaySlider.c_str(), 5.f, &TriggerBot::TriggerDelay, &DelayMin, &DelayMax, "%d ms");
                        PutSliderInt(Text::Trigger::FakeShotSlider.c_str(), 5.f, &TriggerBot::ShotDuration, &DurationMin, &DurationMax, "%d ms");
                    }

                    ImGui::Columns(1);
                }
                else if (MenuConfig::WCS.MenuPage == 2) {
                    // Misc Settings Tab
                    ImGui::Columns(2, nullptr, false);

                    // First section: Miscellaneous Features
                    ::DrawCardHeader("Features", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // Misc controls...
                    int FovMin = 60, FovMax = 140;
                    int NightMin = 0, NightMax = 150;
                    float FlashMin = 0.f, FlashMax = 255.f;

                    PutSwitch(Text::Misc::bmbTimer.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::bmbTimer, true, "###bmbTimerCol", reinterpret_cast<float*>(&MiscCFG::BombTimerCol));
                    PutSwitch(Text::Misc::SpecList.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::SpecList);
                    PutSwitch(Text::Misc::Watermark.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::WaterMark);
                    PutSwitch(Text::Misc::HeadshotLine.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::ShowHeadShootLine);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
                    ImGui::TextDisabled(Text::Misc::HitSound.c_str());
                    ImGui::SameLine();
                    AlignRight(160.f);
                    ImGui::SetNextItemWidth(160.f);
                    ImGui::Combo("###HitSounds", &MiscCFG::HitSound, "None\0Neverlose\0Skeet\0");
                    PutSwitch(Text::Misc::HitMerker.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::HitMarker);
                    PutSwitch(Text::Misc::BunnyHop.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::BunnyHop);
                    PutSwitch(Text::Misc::SniperCrosshair.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::SniperCrosshair, true, "###sniperCrosshair", reinterpret_cast<float*>(&MiscCFG::SniperCrosshairColor));

                    ImGui::NextColumn();

                    // Second section: Global Settings
                    ::DrawCardHeader("Global Settings", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    ImGui::TextDisabled(Text::Misc::MenuKey.c_str());
                    ImGui::SameLine();
                    AlignRight(70.f);
                    if (ImGui::Button(Text::Misc::HotKey.c_str(), { 70.f, 25.f }))
                    {
                        std::thread([&]() {
                            KeyMgr::GetPressedKey(MenuConfig::HotKey, Text::Misc::HotKey);
                            }).detach();
                    }
                    PutSwitch(Text::Misc::SpecCheck.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MenuConfig::WorkInSpec);
                    PutSwitch(Text::Misc::TeamCheck.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MenuConfig::TeamCheck);
                    PutSwitch(Text::Misc::AntiRecord.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MenuConfig::BypassOBS);

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Third section: About
                    ::DrawCardHeader("About", ImGui::GetCursorScreenPos(), drawList);
                    ImGui::Spacing();

                    // Version info with accent color
                    ImGui::TextColored(BorderColorVec4, "HuhtalaHook");
                    ImGui::TextDisabled("by Darg");

                    ImGui::Spacing();

                    // Modern flat buttons with hover effects
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                    if (ImGui::Button("Source Code", { 125.f, 30.f }))
                        Gui.OpenWebpage("");
                    ImGui::SameLine();
                    if (ImGui::Button("Contact Author", { 125.f, 30.f }))
                        Gui.OpenWebpage("");

                    ImGui::Spacing();

                    // Danger zone buttons with different colors
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.05f, 0.05f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.10f, 0.10f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.15f, 0.15f, 0.8f));

                    if (ImGui::Button("Unhook", { 125.f, 30.f }))
                        Init::Client::Exit();

                    ImGui::SameLine();

                    if (ImGui::Button("Clear Traces", { 125.f, 30.f }))
                    {
                        Misc::CleanTraces();
                        Init::Client::Exit();
                    }

                    ImGui::PopStyleColor(3);
                    ImGui::PopStyleVar();

                    ImGui::Columns(1);
                }
                else if (MenuConfig::WCS.MenuPage == 3) {
                    // Configuration Tab
                    // Use the dedicated config menu rendering (separated into its own function)
                    ConfigMenu::RenderCFGmenu();
                }

            } ImGui::EndChild();
            ImGui::PopStyleVar(); // Pop fade alpha
        } ImGui::End();

        LoadDefaultConfig();
    }

    // Modernized switch button with animations
    void SwitchButton(const char* str_id, bool* v)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        float Height = ImGui::GetFrameHeight();
        float Width = Height * 1.7f;
        float Radius = Height / 2 - 2;

        ImGui::InvisibleButton(str_id, ImVec2(Width, Height));
        if (ImGui::IsItemClicked())
            *v = !(*v);

        // Animation
        float t = *v ? 1.0f : 0.f;
        ImGuiContext& g = *GImGui;
        float AnimationSpeed = 0.06f; // Slightly faster for more responsive feel
        if (g.LastActiveId == g.CurrentWindow->GetID(str_id))
        {
            float T_Animation = ImSaturate(g.LastActiveIdTimer / AnimationSpeed);
            t = *v ? (T_Animation) : (1.0f - T_Animation);
        }

        // Modern colors with midnight theme
        ImColor bgColor = ImColor(0.10f, 0.10f, 0.14f, 0.80f);
        ImColor activeColor = ImColor(0.10f, 0.12f, 0.28f, t * 0.80f + 0.20f);

        // Draw shadow
        DrawList->AddRectFilled(
            ImVec2(p.x + 1, p.y + Height * 0.15f + 1),
            ImVec2(p.x + Width + 1, p.y + Height * 0.85f + 1),
            ImColor(0.0f, 0.0f, 0.0f, 0.20f),
            Height / 2
        );

        // Background with rounded corners
        DrawList->AddRectFilled(
            ImVec2(p.x, p.y + Height * 0.15f),
            ImVec2(p.x + Width, p.y + Height * 0.85f),
            bgColor,
            Height / 2
        );

        // Active background
        if (t > 0.0f) {
            DrawList->AddRectFilled(
                ImVec2(p.x, p.y + Height * 0.15f),
                ImVec2(p.x + Width * t, p.y + Height * 0.85f),
                activeColor,
                Height / 2
            );
        }

        // Circle knob with shadow
        float knobPosX = p.x + Radius + t * (Width - Radius * 2);

        // Knob shadow
        DrawList->AddCircleFilled(
            ImVec2(knobPosX + 1, p.y + Radius + 1),
            Radius - 1,
            ImColor(0.0f, 0.0f, 0.0f, 0.25f),
            12
        );

        // Knob
        DrawList->AddCircleFilled(
            ImVec2(knobPosX, p.y + Radius),
            Radius - 1,
            ImColor(1.0f, 1.0f, 1.0f, 1.0f),
            12
        );

        // Add highlight to knob when active
        if (*v) {
            DrawList->AddCircle(
                ImVec2(knobPosX, p.y + Radius),
                Radius - 3,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.14f, 0.16f, 0.36f, 0.5f)),
                12, 1.0f
            );
        }
    }
}
