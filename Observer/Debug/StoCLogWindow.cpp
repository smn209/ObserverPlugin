#include "StoCLogWindow.h"
#include "../ObserverPlugin.h"

void StoCLogWindow::Draw(ObserverPlugin& plugin, bool& is_visible)
{
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Observer Plugin - StoC Log Options", &is_visible))
    {
        ImGui::Checkbox("Enable StoC logging chat", &plugin.stoc_status);
        ImGui::SameLine(); 
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Master toggle for displaying StoC log messages in the game chat.\nAll events are still recorded internally for export regardless of this setting.");
        }
        
        if (plugin.stoc_status)
        {
            ImGui::Indent();
            auto AddLogCheckbox = [](const char* label, bool* v, const char* tooltip_text) {
                ImGui::Checkbox(label, v);
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(tooltip_text);
                }
            };

            if (ImGui::TreeNode("Skill Events")) {
                AddLogCheckbox("Activations##Skill", &plugin.log_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Finishes##Skill", &plugin.log_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillFinished\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Stops##Skill", &plugin.log_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillStopped\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Instant Activations##Skill", &plugin.log_instant_skills, "Chat Log Toggle\nHandler: ObserverStoC::handleInstantSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Attack Skill Events")) {
                 AddLogCheckbox("Activations##AttackSkill", &plugin.log_attack_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillActivated\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Finishes##AttackSkill", &plugin.log_attack_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillFinished\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Stops##AttackSkill", &plugin.log_attack_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillStopped\nMarker: [ASK]\nFile: attack_skill_events.txt");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Basic Attack Events")) {
                AddLogCheckbox("Starts##BasicAttack", &plugin.log_basic_attack_starts, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStarted\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Finishes##BasicAttack", &plugin.log_basic_attack_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackFinished\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Stops##BasicAttack", &plugin.log_basic_attack_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStopped\nMarker: [ATK]\nFile: basic_attack_events.txt");
               ImGui::TreePop();
            }
            if (ImGui::TreeNode("Combat Events")) {
                AddLogCheckbox("Damage", &plugin.log_damage, "Chat Log Toggle\nHandler: ObserverStoC::handleDamage\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Interrupts", &plugin.log_interrupts, "Chat Log Toggle\nHandler: ObserverStoC::handleInterrupted\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Knockdowns", &plugin.log_knockdowns, "Chat Log Toggle\nHandler: ObserverStoC::handleKnockdown\nMarker: [CMB]\nFile: combat_events.txt");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Agent Events")) {
                AddLogCheckbox("Movement", &plugin.log_movement, "Chat Log Toggle\nHandler: ObserverStoC::handleAgentMovement\nMarker: [AGT]\nFile: agent_events.txt");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Jumbo Messages")) {
                AddLogCheckbox("Base Under Attack##Jumbo", &plugin.log_jumbo_base_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 0\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Guild Lord Under Attack##Jumbo", &plugin.log_jumbo_guild_lord_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 1\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Shrine##Jumbo", &plugin.log_jumbo_captured_shrine, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 3\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Tower##Jumbo", &plugin.log_jumbo_captured_tower, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 5\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Party Defeated##Jumbo", &plugin.log_jumbo_party_defeated, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 6\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Morale Boost##Jumbo", &plugin.log_jumbo_morale_boost, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 9\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Victory##Jumbo", &plugin.log_jumbo_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 16\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Flawless Victory##Jumbo", &plugin.log_jumbo_flawless_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 17\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Unknown Types##Jumbo", &plugin.log_jumbo_unknown, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nUnknown Types\nMarker: [JMB]\nFile: jumbo_messages.txt");
                ImGui::TreePop();
            }
            ImGui::Unindent();
        }
    }
    ImGui::End();
} 