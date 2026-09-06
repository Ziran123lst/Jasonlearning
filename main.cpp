// ============================================================
//  MEGA39's (Switch) — FT UI subsdk  (exlaunch)
//  移植自 PC 版 mod: github.com/vixen256/ps4
//
//  核心四招:
//   1. ChangeSubGameState 重定向: MENU_SWITCH(9,42) -> CS_MENU(6,32)
//   2. GetFtTheme 劫持: 恒返回 theme 变量 (0 = "_ft" 后缀)
//   3. PlayAetLayer 层名白名单追加 "_ft"
//   4. 字符串手术 (menu_txt_* 清空等)
// ============================================================
#include <exl.h>
#include <exlaunch.hpp>
#include <cstring>
#include <cstdio>
#include "diva.h"

// ================= 用户配置 =================
namespace cfg {
    // 主题: 0="_ft"(推荐) 1="_f" 2="_t"  (对应 PC config.toml 的 theme)
    int  theme              = 0;
    // 是否对 AET 层名做 _ft 重命名 (若你的 AET 档案缺 _ft 变体导致界面元素消失, 改为 false)
    bool renameAetLayers    = true;
    // 是否清空 pv_db_switch.txt 让其回退 FT 的 pv_db.txt
    // ⚠ Switch 版默认 false! 开了会丢失 MEGA39's 的 10 首新歌/体感谱面 (PC 曲库全在 pv_db.txt, Switch 不是)
    bool nullPvDbSwitch     = false;
    // 清空 menu_txt_* (PC mod 同款; 这些是 Switch 菜单文字层, FT 菜单用自己的)
    bool nullMenuTxt        = true;
}

// ================= 工具 =================
namespace {

// AET 层名白名单 (照抄 PC mod themeStrings; Switch 数据里不存在的名字匹配不到, 无害)
const char* const g_themeStrings[] = {
    "option_sub_menu_eachsong", "option_sub_menu_allsong", "timing",
    "option_sub_menu_vibration", "option_sub_menu_bgm_volume",
    "option_sub_menu_button_volume", "option_sub_menu_se_volume",
    "option_sub_menu_back",
    "gam_btn_resume", "gam_btn_timing", "gam_btn_retry", "gam_btn_option",
    "gam_btn_back", "gam_btn_back_playlist",
    "nswgam_cmnbg_bg", "nswgam_adv_bg", "press_a_button",
    "footer_01", "option_top_menu loop",
    "fotter01", "fotter02", "fotter03", "fotter04", "fotter05",
    "fotter08", "fotter09",
    "setting_menu_bg_arcade_base_in", "setting_menu_bg_arcade_base_up",
    "setting_menu_bg_arcade_base_down",
    "bg02", "footer_02", "footer_03",
    "nswgam_tshirtsedit_colorselector_bg", "nswgam_tshirtsedit_keyhelp_bg",
    "cmn_win_help", "btn_close", "savedata_warning_dialog",
    "cmn_win_m", "cmn_menu_yes", "cmn_menu_no",
};

// 重命名缓冲环 (防止嵌套调用互相踩)
constexpr int  kRing   = 8;
constexpr int  kBufLen = 96;
char  g_ring[kRing][kBufLen];
int   g_ringIdx = 0;

const char* renameLayer(const char* name) {
    if (!cfg::renameAetLayers || name == nullptr) return name;
    for (auto* s : g_themeStrings) {
        if (strcmp(name, s) == 0) {
            char* buf = g_ring[g_ringIdx];
            g_ringIdx = (g_ringIdx + 1) % kRing;
            snprintf(buf, kBufLen, "%s_ft", name);
            return buf;
        }
    }
    return name;
}

const char* themeSuffix() {
    switch (cfg::theme) {
        case 1:  return "_f";
        case 2:  return "_t";
        default: return "_ft";
    }
}

} // namespace

// ================= 钩子 1: 状态机重定向 (主钥匙) =================
// PC: HOOK ChangeSubGameState 0x1527E49E0 -> Switch: 0x216710
// (9,x)->(6,x); (x,34)->(9,44) 设置页保留 Switch; (x,42)->(6,32)
HOOK_DEFINE_TRAMPOLINE_STATIC(ChangeSubGameState) {
    static void Callback(int state, int subState) {
        using namespace diva;
        if (state == State::MENU_SWITCH) {
            state = State::CS_MENU;
        } else if (subState == SubState::CS_OPTION_MENU) {
            state    = State::MENU_SWITCH;
            subState = SubState::OPTION_MENU_SWITCH;
        } else if (subState == SubState::MENU_SWITCH) {
            state    = State::CS_MENU;
            subState = SubState::CS_MENU;
        }
        Orig(state, subState);
    }
};

// ================= 钩子 2: FT 主题变量 =================
// PC: HOOK GetFtTheme 0x1401D6540 返回 &theme -> Switch: 0x64B840
// 引擎原生后缀机制 (0x7C6B0 一带) 会拿这个值拼 _ft/_f/_t
HOOK_DEFINE_STATIC(GetFtTheme) {
    static int Callback() {
        return cfg::theme;
    }
};

// ================= 钩子 3: AET 层名重命名 =================
// PC: HOOK PlayAetLayer 0x1402CA220 -> Switch: 0x1DB4C0 / 0x1DBCC0
HOOK_DEFINE_TRAMPOLINE_STATIC(PlayAetLayer) {
    static void Callback(int* outId, int sceneId, const char* layerName,
                         int priority, int action) {
        Orig(outId, sceneId, renameLayer(layerName), priority, action);
    }
};

HOOK_DEFINE_TRAMPOLINE_STATIC(PlayAetLayerM) {
    static void Callback(int* outId, int sceneId, const char* layerName, int priority,
                         const char* startMarker, const char* endMarker,
                         const char* unk, int action) {
        Orig(outId, sceneId, renameLayer(layerName), priority,
             startMarker, endMarker, unk, action);
    }
};

// ================= 字符串手术 =================
static void nullString(uintptr_t off) {
    // 把 .rodata 字符串首字节清零 => 引擎请求空名 => 加载失败 => 该层不出现
    exl::patch::CodePatcher p(off);
    p.Write<u8>(0);
}

static void preInit() {
    using namespace diva::str;
    if (cfg::nullMenuTxt) {
        nullString(menu_txt_01);
        nullString(menu_txt_02);
        nullString(menu_txt_03);
        nullString(menu_txt_04);
        nullString(menu_txt_05);
        nullString(menu_txt_base);
    }
    if (cfg::nullPvDbSwitch) {
        nullString(pv_db_switch_txt);
    }
    // PC mod 还改了 cmn_win_help_g/p/r/y -> cmn_win_help 和 cmn_win_g/p -> cmn_win_m
    // Switch ELF 里这些源字符串不存在 (cmn_win_help_g/cmn_win_m 均无), 不移植
}

// ================= 入口 =================
extern "C" void exl_main(void* x0, void* x1) {
    exl::hook::Initialize();

    preInit();

    ChangeSubGameState::InstallAtOffset(diva::off::ChangeSubGameState);
    GetFtTheme::InstallAtOffset(diva::off::GetFtTheme);
    PlayAetLayer::InstallAtOffset(diva::off::PlayAetLayer);
    PlayAetLayerM::InstallAtOffset(diva::off::PlayAetLayerM);

    exl::loader::StartOriginal();
}

extern "C" void exl_entrypoint() {
    exl::patch::Initialize();
    exl_main(nullptr, nullptr);
    exl::patch::Finalize();
}

// ============================================================
//  未移植的 PC mod 钩子 (TODO, 按需求逐个补):
//   - PlayHeaderFooter/commonMenu: 按输入设备切 footer_button_XX_YY 层
//   - customize: footer 修复 + cmnMenu 隐藏 (CustomizeSelInit 0x140687D10)
//   - pvSel: song_list_num 位数修复 / 排序选项修复
//   - result: survival sprite id 修复
//   - genericDialog: 按键动画透明度 / help 页颜色
//   - BGM index 修复 (CsMenu/CsGallery/CsResult 1-4)
//   - gamma (PS4 伽马曲线, 需要 D3D 等价物 -> Switch 是 NVN, 另议)
//   - "Stop returning to ADV from main menu" NOP
//  PC 源文件已全部存到 /tmp/vixen (本会话), 需要哪个报编号
// ============================================================
