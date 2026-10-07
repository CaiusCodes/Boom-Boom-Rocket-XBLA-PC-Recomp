#pragma once
#include "menu_mouse.h"
#include "title_screen_state.h"
#include <vector>
#include <limits>
#include <chrono>

// Only the guest UI thread reads these objects. The window thread publishes
// coordinates, never guest pointers. Bounds come from the native rendered quads.
struct BbrMouseRow {
    uint32_t object,index,type;
    float left,top,right,bottom;
};
static std::vector<BbrMouseRow> g_bbr_mouse_rows;
static bbr::MenuMouseEvent g_bbr_mouse_pending;
static unsigned g_bbr_mouse_steps=0;
static bbr::SongWheel g_bbr_song_wheel;
static bbr::SpinnerClickCycle g_bbr_settings_click_cycle;
bool BBR_IsSettingsScreen();
bool BBR_TakeSettingsClickCycle(uint32_t object) {
    return g_bbr_settings_click_cycle.Take(object);
}
static float BbrGuestFloat(uint8_t* base,uint32_t address) {
    PPCRegister value{}; value.u32=REX_LOAD_U32(address); return value.f32;
}
static bool BBR_InPopupList(uint8_t* base,uint32_t object) {
    if(!REX_LOAD_U8(0x821A1D0D)) return false;
    const uint32_t head=REX_LOAD_U32(0x821D46D0+484+4);
    if(!head) return false;
    uint32_t node=REX_LOAD_U32(head);
    for(unsigned n=0;node && node!=head && n<256;++n) {
        if(REX_LOAD_U32(node+8)==object) return true;
        node=REX_LOAD_U32(node);
    }
    return false;
}
bool BBR_HideControllerHint(uint8_t* base,uint32_t object) {
    if(bbr::physical_controller_connected.load() || bbr::gameplay_screen_active.load()) return false;
    // Only standalone controller hints, not instructional paragraphs or menus.
    const uint32_t id=REX_LOAD_U32(object+752);
    const bool hint=id==0x3E || (id>=0x5F && id<=0x64) || id==0x67 ||
        (id>=0x6B && id<=0x6E) || (id>=0x8E && id<=0x91) || id==0xB4 ||
        id==0xB7 || id==0xCF || (id>=0xD9 && id<=0xDC) || id==0xF2 ||
        id==0xFB || id==0x100 || id==0x111 || id==0x115 || id==0x11A;
    return hint && !BBR_InPopupList(base,object);
}
static uint32_t g_bbr_popup_text=0,g_bbr_popup_matrix=0;
void BBR_BeginPopupText(uint8_t* base,uint32_t object,uint32_t matrix) {
    const uint32_t id=REX_LOAD_U32(object+752);
    g_bbr_popup_text=(id==0x62 || id==0x100) && BBR_InPopupList(base,object)?object:0;
    g_bbr_popup_matrix=matrix;
}
void BBR_EndPopupText() {g_bbr_popup_text=0;}
void BBR_RecordPopupText(uint8_t* base,float width,float height,uint32_t justify) {
    if(!g_bbr_popup_text || g_bbr_mouse_rows.size()>=256 ||
       !std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0) return;
    BbrMouseRow row{g_bbr_popup_text,REX_LOAD_U32(g_bbr_popup_text+752),1,1e9f,1e9f,-1e9f,-1e9f};
    const float left=justify==1?-width*.5f:justify==2?-width:0;
    // Native measured font width, font height, and composed parent/effect matrix.
    for(unsigned corner=0;corner<4;++corner) {
        const float x=left+(corner&1?width:0),y=corner&2?height:0;
        const uint32_t m=g_bbr_popup_matrix;
        const float tx=x*BbrGuestFloat(base,m)+y*BbrGuestFloat(base,m+16)+BbrGuestFloat(base,m+48);
        const float ty=x*BbrGuestFloat(base,m+4)+y*BbrGuestFloat(base,m+20)+BbrGuestFloat(base,m+52);
        if(!std::isfinite(tx)||!std::isfinite(ty)) return;
        row.left=std::min(row.left,tx);row.right=std::max(row.right,tx);
        row.top=std::min(row.top,ty);row.bottom=std::max(row.bottom,ty);
    }
    if(row.right>row.left && row.bottom>row.top) g_bbr_mouse_rows.push_back(row);
}
void BBR_RecordMouseRow(uint8_t* base,uint32_t object,uint32_t index,uint32_t type) {
    if(g_bbr_mouse_rows.size()>=256) return;
    BbrMouseRow row{object,index,type,1e9f,1e9f,-1e9f,-1e9f};
    const uint32_t first=type==2?888:868;
    for(unsigned quad=0;quad<3;++quad) for(unsigned vertex=0;vertex<4;++vertex) {
        // Menu rendering updates end-left, middle, and end-right at slots
        // 0/1/3; slot 2 is a separate effect and is not this row's bounds.
        const unsigned slot=type==2 && quad==2?3:quad;
        const uint32_t p=object+first+slot*152+vertex*32;
        const float x=BbrGuestFloat(base,p),y=BbrGuestFloat(base,p+4);
        if(!std::isfinite(x)||!std::isfinite(y)) return;
        row.left=std::min(row.left,x);row.right=std::max(row.right,x);
        row.top=std::min(row.top,y);row.bottom=std::max(row.bottom,y);
    }
    if(row.right>row.left && row.bottom>row.top) g_bbr_mouse_rows.push_back(row);
}
static void BBR_ClearMouseState() {
    g_bbr_mouse_rows.clear();g_bbr_mouse_pending={};g_bbr_mouse_steps=0;bbr::ClearMenuMouse();
    g_bbr_song_wheel={};
    g_bbr_settings_click_cycle.Clear();
}
static void BBR_MouseUiFrame(PPCContext& ctx,uint8_t* base,uint32_t manager) {
    auto rows=std::move(g_bbr_mouse_rows);g_bbr_mouse_rows.clear();
    if(bbr::gameplay_screen_active.load() || bbr::overlay_captures_input.load()) {
        BBR_ClearMouseState();return;
    }
    // Validate against the current live list, including modal popups. A deleted
    // row or the underlying Settings menu must never receive a popup click.
    const uint32_t list=0x821D46D0+(REX_LOAD_U8(0x821A1D0D)?484:468);
    const uint32_t head=REX_LOAD_U32(list+4);
    if(!head) {BBR_ClearMouseState();return;}
    std::vector<uint32_t> live;
    uint32_t node=REX_LOAD_U32(head);
    for(unsigned n=0;node && node!=head && n<256;++n) {
        live.push_back(REX_LOAD_U32(node+8));node=REX_LOAD_U32(node);
    }
    rows.erase(std::remove_if(rows.begin(),rows.end(),[&](const BbrMouseRow& r) {
        return std::find(live.begin(),live.end(),r.object)==live.end();
    }),rows.end());
    // Modal clicks are consumed even before their first rendered frame, so
    // clicking blank space never falls back to an unconditional virtual A.
    bbr::mouse_menu_active.store(!rows.empty() || REX_LOAD_U8(0x821A1D0D));
    auto event=bbr::TakeMenuMouse();
    if(bbr::song_select_active.load() && !REX_LOAD_U8(0x821A1D0D)) {
        // A scrolling list moves underneath the pointer. Hover-to-select
        // would continually chase those moving rows. Wheel changes selection;
        // a left click confirms the already-highlighted song without a jump.
        const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        g_bbr_song_wheel.Add(bbr::TakeMenuWheel(),now);
        g_bbr_mouse_pending={};
        if(REX_LOAD_U32(manager+2560)!=0) {g_bbr_song_wheel={};return;}
        const int direction=g_bbr_song_wheel.Next(now);
        const uint32_t command=event.click && event.point.valid?0x820049B8:
            direction>0?0x820049D0:direction<0?0x820049C4:0;
        if(command) {
            const auto saved=ctx;
            ctx.r3.u32=manager;ctx.r4.u32=command;ctx.f1.f64=0;ctx.f2.f64=0;
            sub_820A3808(ctx,base);ctx=saved;
        }
        return;
    }
    bbr::TakeMenuWheel();g_bbr_song_wheel={};
    if(event.dirty) {g_bbr_mouse_pending=event;g_bbr_mouse_steps=0;}
    if(!g_bbr_mouse_pending.dirty) return;
    // Keyboard/controller input wins; do not fight a stationary mouse cursor.
    if(REX_LOAD_U32(manager+2560)!=0 || ++g_bbr_mouse_steps>16) {g_bbr_mouse_pending={};return;}
    const auto point=g_bbr_mouse_pending.point;
    const BbrMouseRow* target=nullptr;
    if(point.valid) for(const auto& row:rows) {
        if(point.x>=row.left && point.x<row.right && point.y>=row.top && point.y<row.bottom &&
           REX_LOAD_U8(row.object+688)!=0) target=&row;
    }
    if(!target) {g_bbr_mouse_pending={};return;}
    const uint32_t object=target->object;
    const auto saved=ctx;
    auto action=[&](uint32_t text) {
        ctx.r3.u32=manager;ctx.r4.u32=text;ctx.f1.f64=0;ctx.f2.f64=0;
        sub_820A3808(ctx,base);ctx=saved;
    };
    if(target->type==1) {
        if(g_bbr_mouse_pending.click) action(target->index==0x62?0x820049AC:0x820049B8);
    } else if(target->type==2) {
        const uint32_t items=REX_LOAD_U32(object+692),count=REX_LOAD_U32(object+696);
        if(!items || target->index>=count || !REX_LOAD_U8(items+target->index*116+112)) {
            g_bbr_mouse_pending={};return;
        }
        if(REX_LOAD_U32(object+700)!=target->index) {
            ctx.r3.u32=object;ctx.r4.u32=REX_LOAD_U32(items+target->index*116+4);
            sub_82094928(ctx,base);ctx=saved;
        }
        if(g_bbr_mouse_pending.click) action(0x820049B8); // BUTTON_NEXT
    } else {
        // Drive the original Lua selection state, rather than changing only
        // the visual highlight and leaving the next Up/Down action out of sync.
        uint32_t selected=0;
        for(const auto& row:rows) if(row.type==6 && REX_LOAD_U8(row.object+687)) selected=row.object;
        if(selected && selected!=object) {
            action(BbrGuestFloat(base,selected+8)<BbrGuestFloat(base,object+8)?0x820049C4:0x820049D0);
            return; // Confirm only after the selected row has actually changed.
        }
        if(g_bbr_mouse_pending.click) {
            const uint32_t count=REX_LOAD_U32(object+696);
            if(count==1) action(0x820049B8); // Reset display
            else if(BBR_IsSettingsScreen() && !REX_LOAD_U8(0x821A1D0D)) {
                g_bbr_settings_click_cycle.Arm(object);
                action(0x82004658); // Advance, regardless of click position.
            }
            else action(point.x<(target->left+target->right)*.5f?0x82004668:0x82004658);
        }
    }
    g_bbr_mouse_pending={};
}
