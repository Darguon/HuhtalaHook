#pragma once
#include "../Core/Config.h"
#include "../Game/Entity.h"
#include "../Features/Aimbot.h"
#include "../Features/TriggerBot.h"
#include "../Resources/Language.hpp"
#include "../OS-ImGui/OS-ImGui.h"
#include <vector>
#include <string>
#include <algorithm>

namespace KeybindStatusDisplay 
{
    // Structure to hold keybind state information
    struct KeybindState {
        int keyCode;
        std::string name;
        bool isActive;
        std::string keyName;
    };

    // Vector to track all keybind states
    inline std::vector<KeybindState> keybindStates = {
        { 0, "Menu Toggle", false, "" },
        { 0, "Aimbot", false, "" },
        { 0, "Trigger Bot", false, "" },
        { 0, "Bunny Hop", false, "" }
    };

    // Update keybind states
    inline void UpdateKeybindStates()
    {
        // Update Menu Toggle
        keybindStates[0].keyCode = MenuConfig::HotKey;
        keybindStates[0].keyName = Text::Misc::HotKey;
        keybindStates[0].isActive = MenuConfig::ShowMenu;

        // Update Aimbot
        keybindStates[1].keyCode = AimControl::HotKey;
        keybindStates[1].keyName = Text::Aimbot::HotKey;
        keybindStates[1].isActive = LegitBotConfig::AimBot && 
            (LegitBotConfig::AimAlways || (GetAsyncKeyState(AimControl::HotKey) & 0x8000));

        // Update Trigger Bot
        keybindStates[2].keyCode = TriggerBot::HotKey;
        keybindStates[2].keyName = Text::Trigger::HotKey;
        keybindStates[2].isActive = LegitBotConfig::TriggerBot && 
            (LegitBotConfig::TriggerAlways || (GetAsyncKeyState(TriggerBot::HotKey) & 0x8000));

        // Update Bunny Hop
        keybindStates[3].keyCode = VK_SPACE;
        keybindStates[3].keyName = "SPACE";
        keybindStates[3].isActive = MiscCFG::BunnyHop && (GetAsyncKeyState(VK_SPACE) & 0x8000);
    }

    // Render the keybind status window
    inline void RenderKeybindStatusWindow(const CEntity& LocalEntity)
    {
        // Only show when enabled and either in-game or menu is open
        if (!MiscCFG::KeybindStatus)
            return;
            
        if (LocalEntity.Controller.TeamID == 0 && !MenuConfig::ShowMenu)
            return;

        // Update keybind states
        UpdateKeybindStates();

        // Basic window setup
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        
        // Set fixed position and size for testing
        ImGui::SetNextWindowPos(ImVec2(10.0f, 300.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(170.0f, 150.0f), ImGuiCond_Always);
        
        // Begin window
        ImGui::Begin("Keybind Status", nullptr, flags);
        
        // Simple content
        ImGui::Text("Active Keybinds:");
        ImGui::Separator();
        
        // Display active keybinds
        bool anyActive = false;
        for (const auto& state : keybindStates) {
            if (state.isActive) {
                anyActive = true;
                ImGui::Text("%s [%s]", state.keyName.c_str(), state.name.c_str());
            }
        }
        
        if (!anyActive) {
            ImGui::Text("No active keybinds");
        }
        
        // Store position
        MenuConfig::KeybindWinPos = ImGui::GetWindowPos();
        ImGui::End();
    }
}
