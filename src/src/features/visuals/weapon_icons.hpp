#pragma once
#include <map>
#include <set>
#include <string>
#include <d3d11.h>
#include "core/d3d11_image.hpp"
#include <algorithm>
#include <cstring>

#include "weapon_icons_data.h"

namespace weapon_hashes {
    constexpr DWORD WEAPON_PISTOL = 0x1B06D571;
    constexpr DWORD WEAPON_PISTOL_MK2 = 0xBFE256D4;
    constexpr DWORD WEAPON_COMBATPISTOL = 0x5EF9FEC4;
    constexpr DWORD WEAPON_APPISTOL = 0x22D8FE39;
    constexpr DWORD WEAPON_STUNGUN = 0x3656C8C1;
    constexpr DWORD WEAPON_PISTOL50 = 0x99AEEB3B;
    constexpr DWORD WEAPON_SNSPISTOL = 0xBFD21232;
    constexpr DWORD WEAPON_SNSPISTOL_MK2 = 0x88374054;
    constexpr DWORD WEAPON_HEAVYPISTOL = 0xD205520E;
    constexpr DWORD WEAPON_VINTAGEPISTOL = 0x83839C4;
    constexpr DWORD WEAPON_FLAREGUN = 0x47757124;
    constexpr DWORD WEAPON_MARKSMANPISTOL = 0xDC4DB296;
    constexpr DWORD WEAPON_REVOLVER = 0xC1B3C3D1;
    constexpr DWORD WEAPON_REVOLVER_MK2 = 0xCB96392F;
    constexpr DWORD WEAPON_DOUBLEACTION = 0x97EA20B8;
    constexpr DWORD WEAPON_RAYPISTOL = 0xAF3696A1;
    constexpr DWORD WEAPON_CERAMICPISTOL = 0x2B5EF5EC;
    constexpr DWORD WEAPON_NAVYREVOLVER = 0x917F6C8C;
    constexpr DWORD WEAPON_GADGETPISTOL = 0x57A4368C;

    constexpr DWORD WEAPON_MICROSMG = 0x13532244;
    constexpr DWORD WEAPON_SMG = 0x2BE6766B;
    constexpr DWORD WEAPON_SMG_MK2 = 0x78A97CD0;
    constexpr DWORD WEAPON_ASSAULTSMG = 0xEFE7E2DF;
    constexpr DWORD WEAPON_COMBATPDW = 0xA3D4D34;
    constexpr DWORD WEAPON_MACHINEPISTOL = 0xDB1AA450;
    constexpr DWORD WEAPON_MINISMG = 0xBD248B55;
    constexpr DWORD WEAPON_RAYCARBINE = 0x476BF155;

    constexpr DWORD WEAPON_PUMPSHOTGUN = 0x1D073A89;
    constexpr DWORD WEAPON_PUMPSHOTGUN_MK2 = 0x555AF99A;
    constexpr DWORD WEAPON_SAWNOFFSHOTGUN = 0x7846A318;
    constexpr DWORD WEAPON_ASSAULTSHOTGUN = 0xE284C527;
    constexpr DWORD WEAPON_BULLPUPSHOTGUN = 0x9D61E50F;
    constexpr DWORD WEAPON_MUSKET = 0xA89CB99E;
    constexpr DWORD WEAPON_HEAVYSHOTGUN = 0x3AABBBAA;
    constexpr DWORD WEAPON_DBSHOTGUN = 0xEF951FBB;
    constexpr DWORD WEAPON_AUTOSHOTGUN = 0x12E82D3D;
    constexpr DWORD WEAPON_COMBATSHOTGUN = 0x5A96BA4;

    constexpr DWORD WEAPON_ASSAULTRIFLE = 0xBFEFFF6D;
    constexpr DWORD WEAPON_ASSAULTRIFLE_MK2 = 0x394F415C;
    constexpr DWORD WEAPON_CARBINERIFLE = 0x83BF0278;
    constexpr DWORD WEAPON_CARBINERIFLE_MK2 = 0xFAD1F1C9;
    constexpr DWORD WEAPON_ADVANCEDRIFLE = 0xAF113F99;
    constexpr DWORD WEAPON_SPECIALCARBINE = 0xC0A3098D;
    constexpr DWORD WEAPON_SPECIALCARBINE_MK2 = 0x969C3D67;
    constexpr DWORD WEAPON_BULLPUPRIFLE = 0x7F229F94;
    constexpr DWORD WEAPON_BULLPUPRIFLE_MK2 = 0x84D6FAFD;
    constexpr DWORD WEAPON_COMPACTRIFLE = 0x624FE830;
    constexpr DWORD WEAPON_MILITARYRIFLE = 0x9D1F17E6;
    constexpr DWORD WEAPON_HEAVYRIFLE = 0x84EA1D5E;

    constexpr DWORD WEAPON_MG = 0x9D07F764;
    constexpr DWORD WEAPON_COMBATMG = 0x7FD62962;
    constexpr DWORD WEAPON_COMBATMG_MK2 = 0xDBBD7280;
    constexpr DWORD WEAPON_GUSENBERG = 0x61012683;

    constexpr DWORD WEAPON_SNIPERRIFLE = 0x05FC3C11;
    constexpr DWORD WEAPON_HEAVYSNIPER = 0x0C472FE2;
    constexpr DWORD WEAPON_HEAVYSNIPER_MK2 = 0xA914799;
    constexpr DWORD WEAPON_MARKSMANRIFLE = 0xC734385A;
    constexpr DWORD WEAPON_MARKSMANRIFLE_MK2 = 0x6A6C02E0;

    constexpr DWORD WEAPON_RPG = 0xB1CA77B1;
    constexpr DWORD WEAPON_GRENADELAUNCHER = 0xA284510B;
    constexpr DWORD WEAPON_MINIGUN = 0x42BF8A85;
    constexpr DWORD WEAPON_FIREWORK = 0x7F7497E5;
    constexpr DWORD WEAPON_RAILGUN = 0x6D544C99;
    constexpr DWORD WEAPON_COMPACTLAUNCHER = 0x781FE4A;
    constexpr DWORD WEAPON_RAYMINIGUN = 0xB62D1F67;

    constexpr DWORD WEAPON_SMOKEGRENADE = 0xFDBC8A50;
    constexpr DWORD WEAPON_STONE_HATCHET = 0x3813FC08;
}

namespace weapon_icons {
    inline bool icons_available = true;
    
    inline std::map<DWORD, std::string> weapon_to_icon = {
        { weapon_hashes::WEAPON_PISTOL, "weapon_pistol" },
        { weapon_hashes::WEAPON_PISTOL_MK2, "weapon_pistol_mk2" },
        { weapon_hashes::WEAPON_COMBATPISTOL, "weapon_combatpistol" },
        { weapon_hashes::WEAPON_APPISTOL, "weapon_appistol" },
        { weapon_hashes::WEAPON_STUNGUN, "weapon_stungun" },
        { weapon_hashes::WEAPON_PISTOL50, "weapon_pistol50" },
        { weapon_hashes::WEAPON_SNSPISTOL, "weapon_snspistol" },
        { weapon_hashes::WEAPON_SNSPISTOL_MK2, "weapon_snspistol_mk2" },
        { weapon_hashes::WEAPON_HEAVYPISTOL, "weapon_heavypistol" },
        { weapon_hashes::WEAPON_VINTAGEPISTOL, "weapon_vintagepistol" },
        { weapon_hashes::WEAPON_FLAREGUN, "weapon_flaregun" },
        { weapon_hashes::WEAPON_MARKSMANPISTOL, "weapon_marksmanpistol" },
        { weapon_hashes::WEAPON_REVOLVER, "weapon_revolver" },
        { weapon_hashes::WEAPON_REVOLVER_MK2, "weapon_revolver_mk2" },
        { weapon_hashes::WEAPON_DOUBLEACTION, "weapon_doubleaction" },
        { weapon_hashes::WEAPON_RAYPISTOL, "weapon_raypistol" },
        { weapon_hashes::WEAPON_CERAMICPISTOL, "weapon_ceramicpistol" },
        { weapon_hashes::WEAPON_NAVYREVOLVER, "weapon_navyrevolver" },
        { weapon_hashes::WEAPON_GADGETPISTOL, "weapon_gadgetpistol" },
        
        { weapon_hashes::WEAPON_MICROSMG, "weapon_microsmg" },
        { weapon_hashes::WEAPON_SMG, "weapon_smg" },
        { weapon_hashes::WEAPON_SMG_MK2, "weapon_smg_mk2" },
        { weapon_hashes::WEAPON_ASSAULTSMG, "weapon_assaultsmg" },
        { weapon_hashes::WEAPON_COMBATPDW, "weapon_combatpdw" },
        { weapon_hashes::WEAPON_MACHINEPISTOL, "weapon_machinepistol" },
        { weapon_hashes::WEAPON_MINISMG, "weapon_minismg" },
        { weapon_hashes::WEAPON_RAYCARBINE, "weapon_raycarbine" },
        
        { weapon_hashes::WEAPON_PUMPSHOTGUN, "weapon_pumpshotgun" },
        { weapon_hashes::WEAPON_PUMPSHOTGUN_MK2, "weapon_pumpshotgun_mk2" },
        { weapon_hashes::WEAPON_SAWNOFFSHOTGUN, "weapon_sawnoffshotgun" },
        { weapon_hashes::WEAPON_ASSAULTSHOTGUN, "weapon_assaultshotgun" },
        { weapon_hashes::WEAPON_BULLPUPSHOTGUN, "weapon_bullpupshotgun" },
        { weapon_hashes::WEAPON_MUSKET, "weapon_musket" },
        { weapon_hashes::WEAPON_HEAVYSHOTGUN, "weapon_heavyshotgun" },
        { weapon_hashes::WEAPON_DBSHOTGUN, "weapon_dbshotgun" },
        { weapon_hashes::WEAPON_AUTOSHOTGUN, "weapon_autoshotgun" },
        { weapon_hashes::WEAPON_COMBATSHOTGUN, "weapon_combatshotgun" },
        
        { weapon_hashes::WEAPON_ASSAULTRIFLE, "weapon_assaultrifle" },
        { weapon_hashes::WEAPON_ASSAULTRIFLE_MK2, "weapon_assaultrifle_mk2" },
        { weapon_hashes::WEAPON_CARBINERIFLE, "weapon_carbinerifle" },
        { weapon_hashes::WEAPON_CARBINERIFLE_MK2, "weapon_carbinerifle_mk2" },
        { weapon_hashes::WEAPON_ADVANCEDRIFLE, "weapon_advancedrifle" },
        { weapon_hashes::WEAPON_SPECIALCARBINE, "weapon_specialcarbine" },
        { weapon_hashes::WEAPON_SPECIALCARBINE_MK2, "weapon_specialcarbine_mk2" },
        { weapon_hashes::WEAPON_BULLPUPRIFLE, "weapon_bullpuprifle" },
        { weapon_hashes::WEAPON_BULLPUPRIFLE_MK2, "weapon_bullpuprifle_mk2" },
        { weapon_hashes::WEAPON_COMPACTRIFLE, "weapon_compactrifle" },
        { weapon_hashes::WEAPON_MILITARYRIFLE, "weapon_militaryrifle" },
        { weapon_hashes::WEAPON_HEAVYRIFLE, "weapon_heavyrifle" },
        
        { weapon_hashes::WEAPON_MG, "weapon_mg" },
        { weapon_hashes::WEAPON_COMBATMG, "weapon_combatmg" },
        { weapon_hashes::WEAPON_COMBATMG_MK2, "weapon_combatmg_mk2" },
        { weapon_hashes::WEAPON_GUSENBERG, "weapon_gusenberg" },
        
        { weapon_hashes::WEAPON_SNIPERRIFLE, "weapon_sniperrifle" },
        { weapon_hashes::WEAPON_HEAVYSNIPER, "weapon_heavysniper" },
        { weapon_hashes::WEAPON_HEAVYSNIPER_MK2, "weapon_heavysniper_mk2" },
        { weapon_hashes::WEAPON_MARKSMANRIFLE, "weapon_marksmanrifle" },
        { weapon_hashes::WEAPON_MARKSMANRIFLE_MK2, "weapon_marksmanrifle_mk2" },
        
        { weapon_hashes::WEAPON_RPG, "weapon_rpg" },
        { weapon_hashes::WEAPON_GRENADELAUNCHER, "weapon_grenadelauncher" },
        { weapon_hashes::WEAPON_MINIGUN, "weapon_minigun" },
        { weapon_hashes::WEAPON_FIREWORK, "weapon_firework" },
        { weapon_hashes::WEAPON_RAILGUN, "weapon_railgun" },
        { weapon_hashes::WEAPON_COMPACTLAUNCHER, "weapon_compactlauncher" },
        { weapon_hashes::WEAPON_RAYMINIGUN, "weapon_rayminigun" },
        
        { weapon_hashes::WEAPON_SMOKEGRENADE, "weapon_smokegrenade" },
        { weapon_hashes::WEAPON_STONE_HATCHET, "weapon_stone_hatchet" },
    };
    
    inline std::map<std::string, DWORD> weapon_name_to_hash = {
        { "PISTOL", weapon_hashes::WEAPON_PISTOL },
        { "PISTOL MK II", weapon_hashes::WEAPON_PISTOL_MK2 },
        { "COMBAT PISTOL", weapon_hashes::WEAPON_COMBATPISTOL },
        { "AP PISTOL", weapon_hashes::WEAPON_APPISTOL },
        { "STUN GUN", weapon_hashes::WEAPON_STUNGUN },
        { "PISTOL .50", weapon_hashes::WEAPON_PISTOL50 },
        { "SNS PISTOL", weapon_hashes::WEAPON_SNSPISTOL },
        { "SNS PISTOL MK II", weapon_hashes::WEAPON_SNSPISTOL_MK2 },
        { "HEAVY PISTOL", weapon_hashes::WEAPON_HEAVYPISTOL },
        { "VINTAGE PISTOL", weapon_hashes::WEAPON_VINTAGEPISTOL },
        { "FLARE GUN", weapon_hashes::WEAPON_FLAREGUN },
        { "MARKSMAN PISTOL", weapon_hashes::WEAPON_MARKSMANPISTOL },
        { "REVOLVER", weapon_hashes::WEAPON_REVOLVER },
        { "REVOLVER MK II", weapon_hashes::WEAPON_REVOLVER_MK2 },
        { "DOUBLE-ACTION REVOLVER", weapon_hashes::WEAPON_DOUBLEACTION },
        { "DOUBLE ACTION REVOLVER", weapon_hashes::WEAPON_DOUBLEACTION },
        { "UP-N-ATOMIZER", weapon_hashes::WEAPON_RAYPISTOL },
        { "CERAMIC PISTOL", weapon_hashes::WEAPON_CERAMICPISTOL },
        { "NAVY REVOLVER", weapon_hashes::WEAPON_NAVYREVOLVER },
        { "PERICO PISTOL", weapon_hashes::WEAPON_GADGETPISTOL },
        
        { "MICRO SMG", weapon_hashes::WEAPON_MICROSMG },
        { "MICROSMG", weapon_hashes::WEAPON_MICROSMG },
        { "SMG", weapon_hashes::WEAPON_SMG },
        { "SMG MK II", weapon_hashes::WEAPON_SMG_MK2 },
        { "ASSAULT SMG", weapon_hashes::WEAPON_ASSAULTSMG },
        { "ASSAULTSMG", weapon_hashes::WEAPON_ASSAULTSMG },
        { "COMBAT PDW", weapon_hashes::WEAPON_COMBATPDW },
        { "COMBATPDW", weapon_hashes::WEAPON_COMBATPDW },
        { "MACHINE PISTOL", weapon_hashes::WEAPON_MACHINEPISTOL },
        { "MACHINEPISTOL", weapon_hashes::WEAPON_MACHINEPISTOL },
        { "MINI SMG", weapon_hashes::WEAPON_MINISMG },
        { "MINISMG", weapon_hashes::WEAPON_MINISMG },
        { "UNHOLY HELLBRINGER", weapon_hashes::WEAPON_RAYCARBINE },
        
        { "PUMP SHOTGUN", weapon_hashes::WEAPON_PUMPSHOTGUN },
        { "PUMPSHOTGUN", weapon_hashes::WEAPON_PUMPSHOTGUN },
        { "PUMP SHOTGUN MK II", weapon_hashes::WEAPON_PUMPSHOTGUN_MK2 },
        { "SAWED-OFF SHOTGUN", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "SAWED OFF SHOTGUN", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "SAWNOFFSHOTGUN", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "ASSAULT SHOTGUN", weapon_hashes::WEAPON_ASSAULTSHOTGUN },
        { "ASSAULTSHOTGUN", weapon_hashes::WEAPON_ASSAULTSHOTGUN },
        { "BULLPUP SHOTGUN", weapon_hashes::WEAPON_BULLPUPSHOTGUN },
        { "BULLPUPSHOTGUN", weapon_hashes::WEAPON_BULLPUPSHOTGUN },
        { "MUSKET", weapon_hashes::WEAPON_MUSKET },
        { "HEAVY SHOTGUN", weapon_hashes::WEAPON_HEAVYSHOTGUN },
        { "HEAVYSHOTGUN", weapon_hashes::WEAPON_HEAVYSHOTGUN },
        { "DOUBLE BARREL SHOTGUN", weapon_hashes::WEAPON_DBSHOTGUN },
        { "DBSHOTGUN", weapon_hashes::WEAPON_DBSHOTGUN },
        { "SWEEPER SHOTGUN", weapon_hashes::WEAPON_AUTOSHOTGUN },
        { "AUTOSHOTGUN", weapon_hashes::WEAPON_AUTOSHOTGUN },
        { "COMBAT SHOTGUN", weapon_hashes::WEAPON_COMBATSHOTGUN },
        { "COMBATSHOTGUN", weapon_hashes::WEAPON_COMBATSHOTGUN },
        
        { "ASSAULT RIFLE", weapon_hashes::WEAPON_ASSAULTRIFLE },
        { "ASSAULTRIFLE", weapon_hashes::WEAPON_ASSAULTRIFLE },
        { "ASSAULT RIFLE MK II", weapon_hashes::WEAPON_ASSAULTRIFLE_MK2 },
        { "CARBINE RIFLE", weapon_hashes::WEAPON_CARBINERIFLE },
        { "CARBINERIFLE", weapon_hashes::WEAPON_CARBINERIFLE },
        { "CARBINE RIFLE MK II", weapon_hashes::WEAPON_CARBINERIFLE_MK2 },
        { "ADVANCED RIFLE", weapon_hashes::WEAPON_ADVANCEDRIFLE },
        { "ADVANCEDRIFLE", weapon_hashes::WEAPON_ADVANCEDRIFLE },
        { "SPECIAL CARBINE", weapon_hashes::WEAPON_SPECIALCARBINE },
        { "SPECIALCARBINE", weapon_hashes::WEAPON_SPECIALCARBINE },
        { "SPECIAL CARBINE MK II", weapon_hashes::WEAPON_SPECIALCARBINE_MK2 },
        { "BULLPUP RIFLE", weapon_hashes::WEAPON_BULLPUPRIFLE },
        { "BULLPUPRIFLE", weapon_hashes::WEAPON_BULLPUPRIFLE },
        { "BULLPUP RIFLE MK II", weapon_hashes::WEAPON_BULLPUPRIFLE_MK2 },
        { "COMPACT RIFLE", weapon_hashes::WEAPON_COMPACTRIFLE },
        { "COMPACTRIFLE", weapon_hashes::WEAPON_COMPACTRIFLE },
        { "MILITARY RIFLE", weapon_hashes::WEAPON_MILITARYRIFLE },
        { "MILITARYRIFLE", weapon_hashes::WEAPON_MILITARYRIFLE },
        { "HEAVY RIFLE", weapon_hashes::WEAPON_HEAVYRIFLE },
        { "HEAVYRIFLE", weapon_hashes::WEAPON_HEAVYRIFLE },
        
        { "MG", weapon_hashes::WEAPON_MG },
        { "COMBAT MG", weapon_hashes::WEAPON_COMBATMG },
        { "COMBATMG", weapon_hashes::WEAPON_COMBATMG },
        { "COMBAT MG MK II", weapon_hashes::WEAPON_COMBATMG_MK2 },
        { "GUSENBERG SWEEPER", weapon_hashes::WEAPON_GUSENBERG },
        { "GUSENBERG", weapon_hashes::WEAPON_GUSENBERG },
        
        { "SNIPER RIFLE", weapon_hashes::WEAPON_SNIPERRIFLE },
        { "SNIPERRIFLE", weapon_hashes::WEAPON_SNIPERRIFLE },
        { "HEAVY SNIPER", weapon_hashes::WEAPON_HEAVYSNIPER },
        { "HEAVYSNIPER", weapon_hashes::WEAPON_HEAVYSNIPER },
        { "HEAVY SNIPER MK II", weapon_hashes::WEAPON_HEAVYSNIPER_MK2 },
        { "MARKSMAN RIFLE", weapon_hashes::WEAPON_MARKSMANRIFLE },
        { "MARKSMANRIFLE", weapon_hashes::WEAPON_MARKSMANRIFLE },
        { "MARKSMAN RIFLE MK II", weapon_hashes::WEAPON_MARKSMANRIFLE_MK2 },
        
        { "RPG", weapon_hashes::WEAPON_RPG },
        { "GRENADE LAUNCHER", weapon_hashes::WEAPON_GRENADELAUNCHER },
        { "GRENADELAUNCHER", weapon_hashes::WEAPON_GRENADELAUNCHER },
        { "MINIGUN", weapon_hashes::WEAPON_MINIGUN },
        { "FIREWORK LAUNCHER", weapon_hashes::WEAPON_FIREWORK },
        { "FIREWORK", weapon_hashes::WEAPON_FIREWORK },
        { "RAILGUN", weapon_hashes::WEAPON_RAILGUN },
        { "COMPACT GRENADE LAUNCHER", weapon_hashes::WEAPON_COMPACTLAUNCHER },
        { "COMPACTLAUNCHER", weapon_hashes::WEAPON_COMPACTLAUNCHER },
        { "WIDOWMAKER", weapon_hashes::WEAPON_RAYMINIGUN },
        
        { "TEAR GAS", weapon_hashes::WEAPON_SMOKEGRENADE },
        { "SMOKEGRENADE", weapon_hashes::WEAPON_SMOKEGRENADE },
        { "STONE HATCHET", weapon_hashes::WEAPON_STONE_HATCHET },
        
        { "PISTOL50", weapon_hashes::WEAPON_PISTOL50 },
        { "SNSPISTOL", weapon_hashes::WEAPON_SNSPISTOL },
        { "HEAVYPISTOL", weapon_hashes::WEAPON_HEAVYPISTOL },
        { "VINTAGEPISTOL", weapon_hashes::WEAPON_VINTAGEPISTOL },
        { "FLAREGUN", weapon_hashes::WEAPON_FLAREGUN },
        { "COMBATPISTOL", weapon_hashes::WEAPON_COMBATPISTOL },
        { "APPISTOL", weapon_hashes::WEAPON_APPISTOL },
        { "STUNGUN", weapon_hashes::WEAPON_STUNGUN },
        { "CERAMICPISTOL", weapon_hashes::WEAPON_CERAMICPISTOL },
        { "NAVYREVOLVER", weapon_hashes::WEAPON_NAVYREVOLVER },
        { "DOUBLEACTION", weapon_hashes::WEAPON_DOUBLEACTION },
        { "MARKSMANPISTOL", weapon_hashes::WEAPON_MARKSMANPISTOL },
        { "GADGETPISTOL", weapon_hashes::WEAPON_GADGETPISTOL },
        { "RAYPISTOL", weapon_hashes::WEAPON_RAYPISTOL },
        { "RAYCARBINE", weapon_hashes::WEAPON_RAYCARBINE },
        { "RAYMINIGUN", weapon_hashes::WEAPON_RAYMINIGUN },
        
        
        { "HVYPISTOL", weapon_hashes::WEAPON_HEAVYPISTOL },
        { "SNSPISTOL", weapon_hashes::WEAPON_SNSPISTOL },
        { "MKPISTOL", weapon_hashes::WEAPON_MARKSMANPISTOL },
        { "VPISTOL", weapon_hashes::WEAPON_VINTAGEPISTOL },
        { "VNTGPISTOL", weapon_hashes::WEAPON_VINTAGEPISTOL },
        { "CERPISTOL", weapon_hashes::WEAPON_CERAMICPISTOL },
        { "GDGTPISTOL", weapon_hashes::WEAPON_GADGETPISTOL },
        { "CMBTPISTOL", weapon_hashes::WEAPON_COMBATPISTOL },
        { "PISTOLMK2", weapon_hashes::WEAPON_PISTOL_MK2 },
        { "SNSPSTLMK2", weapon_hashes::WEAPON_SNSPISTOL_MK2 },
        
        { "ASLTSMG", weapon_hashes::WEAPON_ASSAULTSMG },
        { "ASSSMG", weapon_hashes::WEAPON_ASSAULTSMG },
        { "MACHPISTOL", weapon_hashes::WEAPON_MACHINEPISTOL },
        { "TACTSMG", weapon_hashes::WEAPON_ASSAULTSMG },
        { "MCROSMG", weapon_hashes::WEAPON_MICROSMG },
        { "SMGMK2", weapon_hashes::WEAPON_SMG_MK2 },
        
        { "HVYSHGN", weapon_hashes::WEAPON_HEAVYSHOTGUN },
        { "HVYSHOTGUN", weapon_hashes::WEAPON_HEAVYSHOTGUN },
        { "BULLSHGN", weapon_hashes::WEAPON_BULLPUPSHOTGUN },
        { "BULLSHOTGUN", weapon_hashes::WEAPON_BULLPUPSHOTGUN },
        { "BULLPUP", weapon_hashes::WEAPON_BULLPUPSHOTGUN },
        { "ASSSHGN", weapon_hashes::WEAPON_ASSAULTSHOTGUN },
        { "ASSSHOTGUN", weapon_hashes::WEAPON_ASSAULTSHOTGUN },
        { "ASLTSHTGN", weapon_hashes::WEAPON_ASSAULTSHOTGUN },
        { "SWNSHGN", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "SAWNSHOTGUN", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "SAWNOFF", weapon_hashes::WEAPON_SAWNOFFSHOTGUN },
        { "DBLSHGN", weapon_hashes::WEAPON_DBSHOTGUN },
        { "DBLSHOTGUN", weapon_hashes::WEAPON_DBSHOTGUN },
        { "DBSHGN", weapon_hashes::WEAPON_DBSHOTGUN },
        { "CMBTSHGN", weapon_hashes::WEAPON_COMBATSHOTGUN },
        { "CMBTSHTGN", weapon_hashes::WEAPON_COMBATSHOTGUN },
        { "PMPSHGN", weapon_hashes::WEAPON_PUMPSHOTGUN },
        { "PUMP", weapon_hashes::WEAPON_PUMPSHOTGUN },
        { "PUMPMK2", weapon_hashes::WEAPON_PUMPSHOTGUN_MK2 },
        
        { "HVYRIFLE", weapon_hashes::WEAPON_HEAVYRIFLE },
        { "ADVRIFLE", weapon_hashes::WEAPON_ADVANCEDRIFLE },
        { "SPLCARBINE", weapon_hashes::WEAPON_SPECIALCARBINE },
        { "SPCARBINE", weapon_hashes::WEAPON_SPECIALCARBINE },
        { "SPCRBN", weapon_hashes::WEAPON_SPECIALCARBINE },
        { "SPCRBMK2", weapon_hashes::WEAPON_SPECIALCARBINE_MK2 },
        { "SPCARBMK2", weapon_hashes::WEAPON_SPECIALCARBINE_MK2 },
        { "SPLCRBMK2", weapon_hashes::WEAPON_SPECIALCARBINE_MK2 },
        { "CMPCTRIFLE", weapon_hashes::WEAPON_COMPACTRIFLE },
        { "CMPTRFL", weapon_hashes::WEAPON_COMPACTRIFLE },
        { "CMPRIFLE", weapon_hashes::WEAPON_COMPACTRIFLE },
        { "MILRIFLE", weapon_hashes::WEAPON_MILITARYRIFLE },
        { "MLTRYRFL", weapon_hashes::WEAPON_MILITARYRIFLE },
        { "BULLRIFLE", weapon_hashes::WEAPON_BULLPUPRIFLE },
        { "BULLRFL", weapon_hashes::WEAPON_BULLPUPRIFLE },
        { "BULLRFLMK2", weapon_hashes::WEAPON_BULLPUPRIFLE_MK2 },
        { "CARBRIFLE", weapon_hashes::WEAPON_CARBINERIFLE },
        { "CRBNRIFLE", weapon_hashes::WEAPON_CARBINERIFLE },
        { "CRBNRFL", weapon_hashes::WEAPON_CARBINERIFLE },
        { "CRBNMK2", weapon_hashes::WEAPON_CARBINERIFLE_MK2 },
        { "CARBMK2", weapon_hashes::WEAPON_CARBINERIFLE_MK2 },
        { "ASSRIFLE", weapon_hashes::WEAPON_ASSAULTRIFLE },
        { "ASSRFL", weapon_hashes::WEAPON_ASSAULTRIFLE },
        { "ASLTRIFLE", weapon_hashes::WEAPON_ASSAULTRIFLE },
        { "ASSRFLMK2", weapon_hashes::WEAPON_ASSAULTRIFLE_MK2 },
        { "ASLTRFLMK2", weapon_hashes::WEAPON_ASSAULTRIFLE_MK2 },
        { "TACTRIFLE", weapon_hashes::WEAPON_MILITARYRIFLE },
        { "PRECRIFLE", weapon_hashes::WEAPON_MARKSMANRIFLE },
        
        { "HVYSNIPER", weapon_hashes::WEAPON_HEAVYSNIPER },
        { "HVYSNPR", weapon_hashes::WEAPON_HEAVYSNIPER },
        { "HVYSNPMK2", weapon_hashes::WEAPON_HEAVYSNIPER_MK2 },
        { "HVYSNPRMK2", weapon_hashes::WEAPON_HEAVYSNIPER_MK2 },
        { "MKSMRIFLE", weapon_hashes::WEAPON_MARKSMANRIFLE },
        { "MKSMRFL", weapon_hashes::WEAPON_MARKSMANRIFLE },
        { "MKRIFLE", weapon_hashes::WEAPON_MARKSMANRIFLE },
        { "MKSMMK2", weapon_hashes::WEAPON_MARKSMANRIFLE_MK2 },
        { "MKSMRFLMK2", weapon_hashes::WEAPON_MARKSMANRIFLE_MK2 },
        { "SNPRRIFLE", weapon_hashes::WEAPON_SNIPERRIFLE },
        { "SNPRRFL", weapon_hashes::WEAPON_SNIPERRIFLE },
        { "SNIPERRFL", weapon_hashes::WEAPON_SNIPERRIFLE },
        
        { "CMBTMG", weapon_hashes::WEAPON_COMBATMG },
        { "CMBTMGMK2", weapon_hashes::WEAPON_COMBATMG_MK2 },
        { "GUSENBRG", weapon_hashes::WEAPON_GUSENBERG },
        { "GSNBRG", weapon_hashes::WEAPON_GUSENBERG },
        { "GUSNBRG", weapon_hashes::WEAPON_GUSENBERG },
        
        { "GRENLAUNCH", weapon_hashes::WEAPON_GRENADELAUNCHER },
        { "GRNLNCH", weapon_hashes::WEAPON_GRENADELAUNCHER },
        { "GRNLAUNCH", weapon_hashes::WEAPON_GRENADELAUNCHER },
        { "CMPCTLAUNCH", weapon_hashes::WEAPON_COMPACTLAUNCHER },
        { "CMPCTLNCH", weapon_hashes::WEAPON_COMPACTLAUNCHER },
        { "HOMLNCH", weapon_hashes::WEAPON_RPG },
        { "FIREWRK", weapon_hashes::WEAPON_FIREWORK },
        { "MINIGUNS", weapon_hashes::WEAPON_MINIGUN },
        
        { "HVYREVOLVER", weapon_hashes::WEAPON_REVOLVER },
        { "HVYRVLVR", weapon_hashes::WEAPON_REVOLVER },
        { "RVLVRMK2", weapon_hashes::WEAPON_REVOLVER_MK2 },
        { "DBLACTION", weapon_hashes::WEAPON_DOUBLEACTION },
        { "DBLACTN", weapon_hashes::WEAPON_DOUBLEACTION },
        { "NAVYREV", weapon_hashes::WEAPON_NAVYREVOLVER },
        { "NVYRVLVR", weapon_hashes::WEAPON_NAVYREVOLVER },
    };
    
    inline std::map<std::string, ID3D11ShaderResourceView*> loaded_textures;
    inline bool textures_initialized = false;
    inline ID3D11Device* g_pDevice = 0;
    
    inline std::string normalize_weapon_name(const std::string& name) {
        std::string result = name;
        
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        
        const std::string prefix = "WEAPON_";
        if (result.length() > prefix.length() && result.substr(0, prefix.length()) == prefix) {
            result = result.substr(prefix.length());
        }
        
        std::replace(result.begin(), result.end(), '_', ' ');
        
        size_t start = result.find_first_not_of(' ');
        size_t end = result.find_last_not_of(' ');
        if (start != std::string::npos && end != std::string::npos) {
            result = result.substr(start, end - start + 1);
        }
        
        return result;
    }
    
    inline void init(ID3D11Device* pDevice) {
        if (!pDevice || textures_initialized) return;
        g_pDevice = pDevice;
        textures_initialized = true;
        icons_available = true;
    }
    
    inline ID3D11ShaderResourceView* load_texture(const std::string& icon_name) {
        if (!g_pDevice) return 0;
        
        auto it = loaded_textures.find(icon_name);
        if (it != loaded_textures.end()) {
            return it->second;
        }
        
        const weapon_icons_data::IconData* icon_data = weapon_icons_data::get_icon_data(icon_name.c_str());
        if (!icon_data || !icon_data->data || icon_data->size == 0) {
            loaded_textures[icon_name] = 0;
            return 0;
        }
        
        ID3D11ShaderResourceView* texture = 0;
        HRESULT hr = CreateSRVFromMemory(
            g_pDevice,
            icon_data->data,
            icon_data->size,
            &texture
        );

        if (SUCCEEDED(hr) && texture) {
            loaded_textures[icon_name] = texture;
            return texture;
        }
        
        loaded_textures[icon_name] = 0;
        return 0;
    }
    
    inline ID3D11ShaderResourceView* get_texture_by_hash(DWORD weapon_hash) {
        auto it = weapon_to_icon.find(weapon_hash);
        if (it == weapon_to_icon.end()) {
            return 0;
        }
        return load_texture(it->second);
    }
    
    inline ID3D11ShaderResourceView* get_texture_by_name(const std::string& weapon_name) {
        auto hash_it = weapon_name_to_hash.find(weapon_name);
        if (hash_it != weapon_name_to_hash.end()) {
            return get_texture_by_hash(hash_it->second);
        }
        
        std::string normalized = normalize_weapon_name(weapon_name);
        
        hash_it = weapon_name_to_hash.find(normalized);
        if (hash_it != weapon_name_to_hash.end()) {
            return get_texture_by_hash(hash_it->second);
        }
        
        std::string no_spaces = normalized;
        no_spaces.erase(std::remove(no_spaces.begin(), no_spaces.end(), ' '), no_spaces.end());
        hash_it = weapon_name_to_hash.find(no_spaces);
        if (hash_it != weapon_name_to_hash.end()) {
            return get_texture_by_hash(hash_it->second);
        }
        
        std::string icon_name = weapon_name;
        std::transform(icon_name.begin(), icon_name.end(), icon_name.begin(), ::tolower);
        std::replace(icon_name.begin(), icon_name.end(), ' ', '_');
        
        if (icon_name.length() < 7 || icon_name.substr(0, 7) != "weapon_") {
            icon_name = "weapon_" + icon_name;
        }
        
        return load_texture(icon_name);
    }
    
    inline std::string get_icon_name_by_weapon_name(const std::string& weapon_name) {
        std::string normalized = normalize_weapon_name(weapon_name);
        
        auto hash_it = weapon_name_to_hash.find(normalized);
        if (hash_it == weapon_name_to_hash.end()) {
            return "";
        }
        
        auto icon_it = weapon_to_icon.find(hash_it->second);
        if (icon_it == weapon_to_icon.end()) {
            return "";
        }
        
        return icon_it->second;
    }
    
    inline void cleanup() {
        for (auto& pair : loaded_textures) {
            if (pair.second) {
                pair.second->Release();
            }
        }
        loaded_textures.clear();
        textures_initialized = false;
        g_pDevice = 0;
    }
}
