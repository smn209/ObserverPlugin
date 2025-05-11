#pragma once

#include <ToolboxUIPlugin.h>
#include <GWCA/Constants/Constants.h>
#include <GWCA/GameEntities/Skill.h>
#include <string>
#include <vector>
#include <map>
#include <imgui.h>

class ObserverPlugin;
namespace GW {
    struct Skill;
    namespace Constants { enum class Profession : uint32_t; }
}
struct MatchInfo;
struct AgentInfo;

struct SkillInfo {
    uint32_t skill_id = 0;
    wchar_t name_enc[64] = {0};
    wchar_t name_dec[256] = {0};
    wchar_t desc_enc[64] = {0};
    wchar_t desc_dec[256] = {0};
    wchar_t desc_formatted[512] = {0};
    wchar_t type_str[128] = {0};
    bool info_fetched = false;
    bool is_elite = false;
    bool is_pve = false;
    float activation = 0.0f;
    int recharge = 0;
    int energy_cost = 0;
    int adrenaline = 0;
    int health_cost = 0;
    const char* profession_acronym = "?";
};

class MatchCompositionsWindow {
public:
    MatchCompositionsWindow() = default;
    ~MatchCompositionsWindow() = default;

    void Draw(ObserverPlugin& obs_plugin, bool& is_visible);

private:
    const wchar_t* GetSkillName(GW::Skill* skill, SkillInfo& info);
    const wchar_t* GetFormattedSkillDescription(GW::Skill* skill, SkillInfo& info);
    const wchar_t* GetSkillTypeString(GW::Skill* skill, SkillInfo& info);
    SkillInfo& GetSkillInfo(uint32_t skill_id);
    void DrawSkillTooltip(const SkillInfo& info);

    std::map<uint32_t, SkillInfo> skill_info_cache_;
}; 