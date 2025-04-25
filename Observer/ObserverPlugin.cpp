#include "ObserverPlugin.h"
#include "ObserverStoC.h"
#include "ObserverMatch.h"
#include "ObserverCapture.h"
#include "ObserverLoop.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

#include <Utils/GuiUtils.h>
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <stringapiset.h>
#include <string>

#include <windows.h>
#include <stdio.h>
#include <vector>      
#include <algorithm>   
#include <iomanip> 
#include <sstream>
#include <mutex>

#include <GWCA/GameEntities/Guild.h> 

struct AgentInfo;
struct GuildInfo; 
class ObserverStoC;
class ObserverMatch;
class ObserverCapture;

std::string EscapeWideStringForJSON(const std::wstring& wstr);

std::string EscapeWideStringForJSON(const std::wstring& wstr) {
    std::stringstream ss;
    ss << "\""; // start quote
    for (wchar_t wc : wstr) {
        if (wc >= 32 && wc <= 126) {
            char c = static_cast<char>(wc);
            if (c == '\"') {
                ss << "\\\""; // escape quote
            } else if (c == '\\') {
                ss << "\\\\"; // escape backslash
            } else {
                ss << c; // append printable ASCII character
            }
        } else {
            // ignore non-printable ASCII, control characters, and non-ASCII chars
            continue;
        }
    }
    ss << "\""; // end quote
    return ss.str();
}

ObserverPlugin::ObserverPlugin()
{
    stoc_handler = new ObserverStoC(this);
    match_handler = new ObserverMatch(stoc_handler);
    match_handler->SetOwnerPlugin(this);
    capture_handler = new ObserverCapture();
    loop_handler = new ObserverLoop(this, match_handler);

    // generate an initial default folder name based on current time
    GenerateDefaultFolderName();

    // configure base UI plugin behavior
    can_show_in_main_window = true;
    can_close = true;
    show_menubutton = true;

    // register instance load callback to detect observer mode
    match_handler->RegisterCallbacks();
}

// destructor needs to be defined if we manually delete handlers
ObserverPlugin::~ObserverPlugin()
{
    if (stoc_handler) {
        delete stoc_handler;
        stoc_handler = nullptr;
    }
    if (match_handler) {
        delete match_handler;
        match_handler = nullptr;
    }
    if (capture_handler) {
        delete capture_handler;
        capture_handler = nullptr;
    }
    if (loop_handler) {
        delete loop_handler;
        loop_handler = nullptr;
    }
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ObserverPlugin instance;
    return &instance;
}

void ObserverPlugin::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(stoc_status);
    PLUGIN_LOAD_BOOL(log_skill_activations);
    PLUGIN_LOAD_BOOL(log_skill_finishes);
    PLUGIN_LOAD_BOOL(log_skill_stops);
    PLUGIN_LOAD_BOOL(log_attack_skill_activations);
    PLUGIN_LOAD_BOOL(log_attack_skill_finishes);
    PLUGIN_LOAD_BOOL(log_attack_skill_stops);
    PLUGIN_LOAD_BOOL(log_basic_attack_starts);
    PLUGIN_LOAD_BOOL(log_basic_attack_finishes);
    PLUGIN_LOAD_BOOL(log_basic_attack_stops);
    PLUGIN_LOAD_BOOL(log_interrupts);
    PLUGIN_LOAD_BOOL(log_instant_skills);
    PLUGIN_LOAD_BOOL(log_damage);
    PLUGIN_LOAD_BOOL(log_knockdowns);
    PLUGIN_LOAD_BOOL(log_movement);

    PLUGIN_LOAD_BOOL(log_jumbo_base_under_attack);
    PLUGIN_LOAD_BOOL(log_jumbo_guild_lord_under_attack);
    PLUGIN_LOAD_BOOL(log_jumbo_captured_shrine);
    PLUGIN_LOAD_BOOL(log_jumbo_captured_tower);
    PLUGIN_LOAD_BOOL(log_jumbo_party_defeated);
    PLUGIN_LOAD_BOOL(log_jumbo_morale_boost);
    PLUGIN_LOAD_BOOL(log_jumbo_victory);
    PLUGIN_LOAD_BOOL(log_jumbo_flawless_victory);
    PLUGIN_LOAD_BOOL(log_jumbo_unknown);
    
    PLUGIN_LOAD_BOOL(auto_export_on_match_end);
    PLUGIN_LOAD_BOOL(auto_reset_name_on_match_end);
}

void ObserverPlugin::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    PLUGIN_SAVE_BOOL(stoc_status);
    PLUGIN_SAVE_BOOL(log_skill_activations);
    PLUGIN_SAVE_BOOL(log_skill_finishes);
    PLUGIN_SAVE_BOOL(log_skill_stops);
    PLUGIN_SAVE_BOOL(log_attack_skill_activations);
    PLUGIN_SAVE_BOOL(log_attack_skill_finishes);
    PLUGIN_SAVE_BOOL(log_attack_skill_stops);
    PLUGIN_SAVE_BOOL(log_basic_attack_starts);
    PLUGIN_SAVE_BOOL(log_basic_attack_finishes);
    PLUGIN_SAVE_BOOL(log_basic_attack_stops);
    PLUGIN_SAVE_BOOL(log_interrupts);
    PLUGIN_SAVE_BOOL(log_instant_skills);
    PLUGIN_SAVE_BOOL(log_damage);
    PLUGIN_SAVE_BOOL(log_knockdowns);
    PLUGIN_SAVE_BOOL(log_movement);

    PLUGIN_SAVE_BOOL(log_jumbo_base_under_attack);
    PLUGIN_SAVE_BOOL(log_jumbo_guild_lord_under_attack);
    PLUGIN_SAVE_BOOL(log_jumbo_captured_shrine);
    PLUGIN_SAVE_BOOL(log_jumbo_captured_tower);
    PLUGIN_SAVE_BOOL(log_jumbo_party_defeated);
    PLUGIN_SAVE_BOOL(log_jumbo_morale_boost);
    PLUGIN_SAVE_BOOL(log_jumbo_victory);
    PLUGIN_SAVE_BOOL(log_jumbo_flawless_victory);
    PLUGIN_SAVE_BOOL(log_jumbo_unknown);
    
    PLUGIN_SAVE_BOOL(auto_export_on_match_end);
    PLUGIN_SAVE_BOOL(auto_reset_name_on_match_end);
}

// draws the generic UI settings in the main settings panel
void ObserverPlugin::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings(); // draw base settings (visibility, etc.)
}

const char* ObserverPlugin::Name() const {
    return "Observer Plugin";
}

void ObserverPlugin::AddLogEntry(const wchar_t* entry) {
    if (capture_handler) capture_handler->AddLogEntry(entry);
}

void ObserverPlugin::GenerateDefaultFolderName() {
    auto t = std::time(nullptr);
    std::tm tm; // use a separate tm struct
    errno_t err = localtime_s(&tm, &t); // use localtime_s
    if (err == 0) { // check for errors
        std::ostringstream oss;
        oss << "Match_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
        strncpy_s(export_folder_name, sizeof(export_folder_name), oss.str().c_str(), _TRUNCATE);
    } else {
        // handle error, e.g., use a default fixed name
        strncpy_s(export_folder_name, sizeof(export_folder_name), "Match_Default", _TRUNCATE);
    }
}

void ObserverPlugin::Initialize(
    ImGuiContext* ctx, 
    const ImGuiAllocFns allocator_fns, 
    const HMODULE toolbox_dll
)
{
    // initialize base ui plugin first
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll); 
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer Plugin initialized!"); 
    // register instance load callback to detect observer mode
    match_handler->RegisterCallbacks();
}

void ObserverPlugin::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate(); // signal base first
    if (match_handler) {
        match_handler->RemoveCallbacks(); // this will now also handle removing stoc callbacks if needed
    }
    if (loop_handler) {
        loop_handler->Stop(); // ensure agent loop is stopped
    }
}

// main draw function, called every frame
void ObserverPlugin::Draw(
    IDirect3DDevice9* /*pDevice*/
)
{
    // check visibility via base class
    bool* is_visible_ptr = GetVisiblePtr(); 
    if (!is_visible_ptr || !(*is_visible_ptr)) return; // exit if window not visible
    
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    
    // draw the observer window content
    if (ImGui::Begin(Name(), is_visible_ptr, GetWinFlags())) {
        // display observer status based on match handler flag
        bool is_observing = match_handler && match_handler->IsObserving();
        
        if (is_observing) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OBSERVER MODE ACTIVE");
        } else {
            ImGui::TextDisabled("NOT OBSERVING");
        }
        
        ImGui::Separator();

        if (ImGui::TreeNodeEx("Export Match", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            if (ImGui::Button("Export")) {
                if (strlen(export_folder_name) > 0) {
                    // convert char folder name to wchar_t
                    wchar_t wfoldername[sizeof(export_folder_name)]; // use stack buffer of same size
                    size_t converted = 0;
                    errno_t err = mbstowcs_s(&converted, wfoldername, sizeof(export_folder_name), export_folder_name, _TRUNCATE);

                    if (err == 0) {
                        if (match_handler) {
                           match_handler->ExportLogsToFolder(wfoldername);
                        } else {
                            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error: Match handler not available for export.");
                        }
                    } else {
                         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error converting folder name for export.");
                    }
                } else {
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Please enter a Match Name for export.");
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Exports all captured StoC events and Agent Loop snapshots for the current match to the specified 'captures/<Match Name>/' folder.");
            }
            
            // export folder name input 
            ImGui::InputText("Match Name", export_folder_name, sizeof(export_folder_name));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Sets the name for the subfolder within 'captures/' where logs will be saved.");
            }

            ImGui::SameLine();
            // generate default folder name button
            if (ImGui::Button("Generate##FolderName")) { 
                GenerateDefaultFolderName(); // reset to default time-based name
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Generates a default Match Name based on the current date and time.");
            }

            // add Checkboxes for auto actions 
            ImGui::Checkbox("Auto Export on Match End", &auto_export_on_match_end);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically exports logs when observer mode ends.");
            }
            ImGui::Checkbox("Auto Reset Match Name on Match End", &auto_reset_name_on_match_end);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically generates a new default match name when observer mode ends.");
            }

            ImGui::Unindent();
            ImGui::TreePop(); 
        }

        // note about data persistence
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); 
        ImGui::TextWrapped("Note: Captured match data (StoC events & Agent states) remains in memory after observer mode ends. You can still export the previous match using the 'Export' button above. Data will be cleared automatically when a new observer session begins.");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (ImGui::TreeNodeEx("Capture Status", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            ImGui::Indent();

            // StoC Events Capture Status
            bool stoc_capturing = match_handler && match_handler->IsObserving();
            ImGui::Text("StoC Events Capture:"); ImGui::SameLine();
            if (stoc_capturing) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
            } else {
                ImGui::TextDisabled("Inactive");
            }
             if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Indicates if StoC events are being captured internally.\n(Triggered by entering observer mode)");
            }

            // agents states capture status
            bool loop_running = loop_handler && loop_handler->IsRunning();
            ImGui::Text("Agents States Capture:"); ImGui::SameLine();
            if (loop_running) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Running");
            } else {
                ImGui::TextDisabled("Stopped");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Indicates if the Agents Loop thread is currently capturing agents states snapshots.\n(Triggered by entering observer mode)");
            }

            ImGui::Unindent();
            ImGui::TreePop(); 
        }
        
        if (ImGui::TreeNodeEx("Live Party Info", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            ImGui::Indent();
            if (match_handler) {
                std::lock_guard<std::mutex> lock(match_handler->GetMatchInfo().agents_info_mutex);
                const auto& agents_info = match_handler->GetMatchInfo().agents_info;

                // create a vector of pointers/references to sort *without* copying AgentInfo
                std::vector<const AgentInfo*> agent_list;
                agent_list.reserve(agents_info.size());
                for(const auto& pair : agents_info) {
                    agent_list.push_back(&pair.second);
                }
                // mutex can be released earlier if sorting is slow, but keeping it for simplicity here.
                // if performance becomes an issue, copy relevant sort keys + agent_id, sort that, then use agent_id to get info.

                // sort the vector of pointers
                std::sort(agent_list.begin(), agent_list.end(), [](const AgentInfo* a, const AgentInfo* b) {
                    if (a->party_id != b->party_id) return a->party_id < b->party_id;
                    if (a->type != b->type) return static_cast<int>(a->type) < static_cast<int>(b->type);
                    return a->agent_id < b->agent_id;
                });

                if (agent_list.empty()) {
                    ImGui::TextDisabled("No party data captured yet.");
                } else {
                    // helper to convert AgentType enum to string
                    auto AgentTypeToString = [](AgentType type) -> const char* {
                        switch (type) {
                            case AgentType::PLAYER: return "Player";
                            case AgentType::HERO: return "Hero";
                            case AgentType::HENCHMAN: return "Henchman";
                            case AgentType::OTHER: return "Other";
                            default: return "Unknown";
                        }
                    };

                    uint32_t current_party_id = (uint32_t)-1; 
                    AgentType current_type = (AgentType)-1;
                    bool party_node_open = false;

                    for (const auto* agent_ptr : agent_list) {
                        const AgentInfo& agent = *agent_ptr; 

                        // start new party node if party ID changes
                        if (agent.party_id != current_party_id) {
                            if (party_node_open) {
                                ImGui::Unindent(); // unindent previous type section
                                ImGui::TreePop(); // pop previous party node
                            }
                            current_party_id = agent.party_id;
                            party_node_open = ImGui::TreeNode((void*)(intptr_t)current_party_id, "Party %u", current_party_id);
                            current_type = (AgentType)-1; 
                        }

                        if (party_node_open) {
                            // print type header if type changes within the current party
                            if (agent.type != current_type) {
                                if (current_type != (AgentType)-1) {
                                    ImGui::Unindent(); // unindent previous type section
                                }
                                current_type = agent.type;
                                // count agents of this type in this party (can be pre-calculated if needed)
                                size_t count = 0;
                                for(const auto* other_agent_ptr : agent_list) {
                                    if (other_agent_ptr->party_id == current_party_id && other_agent_ptr->type == current_type) {
                                        count++;
                                    }
                                }
                                ImGui::Text("  %ss (%zu):", AgentTypeToString(current_type), count);
                                ImGui::Indent();
                            }

                            // display current agent info using stringstream
                            std::stringstream ss_display;
                            ss_display << agent.agent_id
                                       << " (L" << agent.level
                                       << " T" << agent.team_id << ")"
                                       << " (G" << agent.guild_id << ")"
                                       << " (" << agent.primary_profession
                                       << "/" << agent.secondary_profession << ")";
                            if (agent.type == AgentType::PLAYER) {
                                ss_display << " [Player#: " << agent.player_number << "]";
                            }
                            // display agent id if name is empty, otherwise the encoded name
                            if (!agent.encoded_name.empty()) {
                                // filter wstring to keep only printable ASCII for display
                                std::string narrow_name_display;
                                narrow_name_display.reserve(agent.encoded_name.length()); // pre-allocate roughly
                                for (wchar_t wc : agent.encoded_name) {
                                    if (wc >= 32 && wc <= 126) {
                                        narrow_name_display += static_cast<char>(wc);
                                    }
                                }
                                ss_display << " Name: " << narrow_name_display;
                            }

                            // add used skills if any
                            if (!agent.used_skill_ids.empty()) {
                                ss_display << " Skills: [";
                                bool first_skill = true;
                                for (uint32_t skill_id : agent.used_skill_ids) {
                                    if (!first_skill) {
                                        ss_display << ", ";
                                    }
                                    ss_display << skill_id;
                                    first_skill = false;
                                }
                                ss_display << "]";
                            }

                            ImGui::TextUnformatted(ss_display.str().c_str());
                        }
                    }

                    // clean up the last opened node/indent if any agents were processed
                    if (party_node_open) {
                        ImGui::Unindent(); // unindent last type section
                        ImGui::TreePop(); // pop last party node
                    }
                }
            } else {
                ImGui::TextDisabled("Match handler not available.");
            }
            ImGui::Unindent();
            ImGui::TreePop(); 
        }

        if (ImGui::TreeNodeEx("Live Guild Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            if (match_handler) {
                std::map<uint16_t, GuildInfo> guilds_info = match_handler->GetMatchInfo().GetGuildsInfoCopy();
                if (guilds_info.empty()) {
                    ImGui::TextDisabled("No guild data captured yet.");
                } else {
                    for (const auto& [guild_id, guild_info] : guilds_info) {
                        std::string narrow_name_display;
                        narrow_name_display.reserve(guild_info.name.length());
                        for (wchar_t wc : guild_info.name) {
                            if (wc >= 32 && wc <= 126) narrow_name_display += static_cast<char>(wc);
                        }

                        std::string narrow_tag_display;
                        narrow_tag_display.reserve(guild_info.tag.length());
                        for (wchar_t wc : guild_info.tag) {
                            if (wc >= 32 && wc <= 126) narrow_tag_display += static_cast<char>(wc);
                        }

                        ImGui::Text("ID: %u, Name: %s, Tag: [%s]", 
                                    guild_info.guild_id, 
                                    narrow_name_display.c_str(), 
                                    narrow_tag_display.c_str());

                        ImGui::Text("  Rank: %u, Rating: %u, Features: %u", 
                                    guild_info.rank, guild_info.rating, guild_info.features);
                        ImGui::Text("  Faction: %s (%u pts), Qualifier Pts: %u",
                                    (guild_info.faction == 0 ? "Kurzick" : (guild_info.faction == 1 ? "Luxon" : "Unknown")), 
                                    guild_info.faction_points, guild_info.qualifier_points);
                        ImGui::Text("  Cape: BG(0x%X), Detail(0x%X), Emblem(0x%X)", 
                                    guild_info.cape.cape_bg_color, guild_info.cape.cape_detail_color, guild_info.cape.cape_emblem_color);
                        ImGui::Text("        Shape(%u), Detail(%u), Emblem(%u), Trim(%u)",
                                    guild_info.cape.cape_shape, guild_info.cape.cape_detail, guild_info.cape.cape_emblem, guild_info.cape.cape_trim);
                    }
                }
            } else {
                 ImGui::TextDisabled("Match handler not available.");
            }
             ImGui::Unindent();
            ImGui::TreePop();
        }

        ImGui::Separator();

        ImGui::Checkbox("Enable StoC logging chat", &stoc_status); 
        ImGui::SameLine(); 
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Master toggle for displaying StoC log messages in the game chat.\nAll events are still recorded internally for export regardless of this setting.");
        }
        
        if (stoc_status)
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
                AddLogCheckbox("Activations##Skill", &log_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Finishes##Skill", &log_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillFinished\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Stops##Skill", &log_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillStopped\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Instant Activations##Skill", &log_instant_skills, "Chat Log Toggle\nHandler: ObserverStoC::handleInstantSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Attack Skill Events")) {
                 AddLogCheckbox("Activations##AttackSkill", &log_attack_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillActivated\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Finishes##AttackSkill", &log_attack_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillFinished\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Stops##AttackSkill", &log_attack_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillStopped\nMarker: [ASK]\nFile: attack_skill_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Basic Attack Events")) {
                AddLogCheckbox("Starts##BasicAttack", &log_basic_attack_starts, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStarted\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Finishes##BasicAttack", &log_basic_attack_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackFinished\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Stops##BasicAttack", &log_basic_attack_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStopped\nMarker: [ATK]\nFile: basic_attack_events.txt");
               ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Combat Events")) {
                AddLogCheckbox("Damage", &log_damage, "Chat Log Toggle\nHandler: ObserverStoC::handleDamage\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Interrupts", &log_interrupts, "Chat Log Toggle\nHandler: ObserverStoC::handleInterrupted\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Knockdowns", &log_knockdowns, "Chat Log Toggle\nHandler: ObserverStoC::handleKnockdown\nMarker: [CMB]\nFile: combat_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Agent Events")) {
                AddLogCheckbox("Movement", &log_movement, "Chat Log Toggle\nHandler: ObserverStoC::handleAgentMovement\nMarker: [AGT]\nFile: agent_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Jumbo Messages")) {
                AddLogCheckbox("Base Under Attack##Jumbo", &log_jumbo_base_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 0\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Guild Lord Under Attack##Jumbo", &log_jumbo_guild_lord_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 1\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Shrine##Jumbo", &log_jumbo_captured_shrine, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 3\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Tower##Jumbo", &log_jumbo_captured_tower, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 5\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Party Defeated##Jumbo", &log_jumbo_party_defeated, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 6\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Morale Boost##Jumbo", &log_jumbo_morale_boost, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 9\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Victory##Jumbo", &log_jumbo_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 16\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Flawless Victory##Jumbo", &log_jumbo_flawless_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 17\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Unknown Types##Jumbo", &log_jumbo_unknown, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nUnknown Types\nMarker: [JMB]\nFile: jumbo_messages.txt");
                ImGui::TreePop();
            }

            ImGui::Unindent();
        }
    }
    ImGui::End();
}
