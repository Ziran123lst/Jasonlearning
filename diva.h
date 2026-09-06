#pragma once
// ============================================================
//  diva.h — MEGA39's (Switch) 引擎偏移与枚举
//  全部偏移 = main.nso 模块相对地址（来自对本体 ELF 的逆向，
//  与用户 dump 的版本严格对应；游戏更新后需重新核对）
// ============================================================
#include <cstdint>

namespace diva {

// ---- 关键函数偏移 (main.nso) ----
namespace off {
    // void ChangeSubGameState(int state, int subState)
    // 状态机总入口：state 0..11 (State枚举)，subState 0..46，MAX 哨兵 = 12/47
    constexpr uintptr_t ChangeSubGameState = 0x216710;

    // int GetFtTheme()  —— 原生实现: 读 *[0xCE24B8]
    // 返回值决定资源名后缀: 1="_f", 2="_t", 其他="_ft"
    constexpr uintptr_t GetFtTheme         = 0x64B840;

    // void PlayAetLayer(int* outId, int sceneId, const char* layerName, int priority, int action)
    constexpr uintptr_t PlayAetLayer       = 0x1DB4C0;

    // void PlayAetLayerM(int* outId, int sceneId, const char* layerName, int priority,
    //                    const char* startMarker, const char* endMarker, const char* unk, int action)
    constexpr uintptr_t PlayAetLayerM      = 0x1DBCC0;
}

// ---- .rodata 字符串偏移（用于字符串手术）----
namespace str {
    constexpr uintptr_t pv_db_switch_txt = 0xACC6DF;  // "pv_db_switch.txt"
    constexpr uintptr_t menu_txt_01      = 0xADD39D;
    constexpr uintptr_t menu_txt_02      = 0xAB9760;
    constexpr uintptr_t menu_txt_03      = 0xADD3A9;
    constexpr uintptr_t menu_txt_04      = 0xAE5E92;
    constexpr uintptr_t menu_txt_05      = 0xAE5E9E;
    constexpr uintptr_t menu_txt_base    = 0xAE7AB0;
}

// ---- 状态枚举（与 PC 版 mod 完全同值，已在 Switch ELF 状态名表验证）----
enum State : int {
    STARTUP         = 0,
    ADVERTISE       = 1,
    GAME            = 2,
    DATA_TEST       = 3,
    TEST_MODE       = 4,
    APP_ERROR       = 5,
    CS_MENU         = 6,   // ← FT 时代主菜单
    CUSTOMIZE       = 7,
    GALLERY         = 8,
    MENU_SWITCH     = 9,   // ← Switch 主菜单
    GAME_SWITCH     = 10,
    TSHIRT_EDIT     = 11,
    MAX             = 12,
};

enum SubState : int {
    SYSTEM_STARTUP     = 1,
    LOGO               = 2,
    TITLE              = 3,
    PV_SEL             = 5,
    PLAYLIST_SEL       = 6,
    GAME_SUB           = 7,
    CS_MENU            = 32,  // ← FT 菜单子状态 (CS_MENU 状态白名单 31-35)
    CS_COMMERCE        = 33,
    CS_OPTION_MENU     = 34,
    CS_CUSTOMIZE_SEL   = 36,
    CS_GALLERY         = 38,
    MENU_SWITCH        = 42,  // ← Switch 菜单子状态
    OPTION_MENU_SWITCH = 44,
    SUB_MAX            = 47,
};

} // namespace diva
