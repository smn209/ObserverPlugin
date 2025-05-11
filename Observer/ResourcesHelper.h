#pragma once

#include <GWCA/Constants/Skills.h>

struct IDirect3DTexture9;

extern "C" __declspec(dllimport) IDirect3DTexture9** __cdecl GetSkillImage(GW::Constants::SkillID skill_id); 