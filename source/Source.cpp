#include "overlay.hpp"
#include "CSSPlayer.hpp"
#include "utils.hpp"

void ESPLoop() {

    hwnd = FindWindowA(NULL, "Counter-Strike 2"); // Target Window With his lpClassName
    OverlayWindow = FindWindow("CEF-OSC-WIDGET", "NVIDIA GeForce Overlay");
    hwnd_active = GetForegroundWindow();

    if (hwnd_active == hwnd) {

        ShowWindow(OverlayWindow, SW_SHOW);
    }
    else
    {
        ShowWindow(OverlayWindow, SW_HIDE);
    }

    if (esp::crosshair)
    {
        g_overlay->draw_line(ScreenCenterX - 15, ScreenCenterY - 15, ScreenCenterX + 15, ScreenCenterY + 15, CrosshairColor);
        g_overlay->draw_line(ScreenCenterX - 15, ScreenCenterY + 15, ScreenCenterX + 15, ScreenCenterY - 15, CrosshairColor);
    }

    ImVec2 size = ImVec2(ScreenCenterX * 2, ScreenCenterY * 2);
    ImVec2 center = ImVec2(ScreenCenterX, ScreenCenterY);

    view_matrix_t viewmatrix = driver::read<view_matrix_t>(client + viewmatrix_Offset);
    uint64_t localplayer = driver::read<uint64_t>(client + localplayer_Offset);
    vec3 localpos = driver::read<vec3>(localplayer + s_Position_Offset);
    int localteam = driver::read<int>(localplayer + s_teamnum_Offset);
    g_dwEntList = driver::read<uint64_t>(client + s_dwEntityList_Offset);
    CCSPlayerController* pPlayerController = nullptr;

    /* @NOTE: Iterate player controllers until we get the first one in the list */
    for (int nId = 0; nId < 32; nId++)
    {
        C_BaseEntity* pBaseEntity = C_BaseEntity::GetBaseEntity(nId);
        if (!pBaseEntity || strcmp(pBaseEntity->m_pEntityIdentity()->m_designerName().c_str(), "cs_player_controller"))
            continue;

        while (pBaseEntity->m_pEntityIdentity()->m_pPrevByClass())
            pBaseEntity = pBaseEntity->m_pEntityIdentity()->m_pPrevByClass();

        pPlayerController = reinterpret_cast<CCSPlayerController*>(pBaseEntity);
        break;
    }

    /* @NOTE: This one controller is first in the entity list, iterate them forward until m_pNextByClass become nullptr */
    for (; pPlayerController; pPlayerController = reinterpret_cast<CCSPlayerController*>(pPlayerController->m_pEntityIdentity()->m_pNextByClass()))
    {
        if (pPlayerController->m_bIsLocalPlayerController())
            continue;

        if (pPlayerController->m_iPawnHealth() <= 0 || pPlayerController->m_iPawnHealth() > 100)
            continue;

        if (pPlayerController->Teamnum() == localteam) 
            continue;

        vec3 playerpos = pPlayerController->pos(pPlayerController->m_hPlayerPawn());
        vec3 pos;

        vec3 head_pos = vec3(playerpos.x, playerpos.y, playerpos.z + 63);
        vec3 head;

        if (!world_to_screen(screensize, playerpos, pos, viewmatrix))
            continue;

        if (!world_to_screen(screensize, head_pos, head, viewmatrix))
            continue;

        /* @NOTE : don't judge all these calculations, I did it very quickly, get inspired by the reasoning but especially not by my way of drawing... sorry lol*/
        float top = head.y;
        float bottom = pos.y;
        float width = abs(top - bottom) / 2.f;
        float left = pos.x - width / 2.f - 1.5f;
        float right = pos.x + width / 2.f + 1.5f;
        float height = abs(abs(pos.y) - abs(pos.y));
        int distance = static_cast<int>(localpos.distance_to(playerpos) / 100);
        auto extend = +5;

        if (esp::health)
        {
            float Height = abs(abs(head.y) - abs(pos.y));
            float Width = height / 2.f;
            float Middle = pos.x - (width / 2.f);

            DrawHealthbars(Middle, top - 5, Width, Height, 2, pPlayerController->m_iPawnHealth());
        }

        if (esp::distance)
        {
            char buffer[0x48];
            sprintf_s(buffer, xorstr_("[%im]"), distance);
            g_overlay->draw_text(pos.x - 12, pos.y, D2D1::ColorF(255, 255, 255, 255), buffer);
        }

        if (esp::box)
        {
            auto extend = +5;
            g_overlay->draw_line(left, top - extend, right, top - extend, D2D1::ColorF(0, 0, 0, 255)); // bottom
            g_overlay->draw_line(left, bottom, right, bottom, D2D1::ColorF(0, 0, 0, 255)); // up
            g_overlay->draw_line(left, top - extend, left, bottom, D2D1::ColorF(0, 0, 0, 255)); // left
            g_overlay->draw_line(right, top - extend, right, bottom, D2D1::ColorF(0, 0, 0, 255)); // right

            auto rect = D2D1_RECT_F();
            rect.bottom = bottom;
            rect.top = top - extend;
            rect.right = right;
            rect.left = left;
            g_overlay->draw_box(rect, D2D1::ColorF(0, 0, 0, 0.2));
        }

        if (esp::healthT)
        {
            float r = 1 - pPlayerController->m_iPawnHealth() * 0.01f;
            float g = pPlayerController->m_iPawnHealth() * 0.01f;

            g_overlay->draw_text(head.x - 15, head.y - 20, D2D1::ColorF(r, g, 0, 255), std::to_string(int(pPlayerController->m_iPawnHealth())).c_str());
        }
    }

}

void RenderMenu()
{
    if (showmenu && rendering)
    {
        g_overlay->draw_text(5, 5, D2D1::ColorF(255, 20, 20, 255), "SHOW/HIDE [INSERT]");

        if (esp::crosshair)
            g_overlay->draw_text(5, 20, D2D1::ColorF(0, 255, 0, 255), "F1 Crosshair : ON");
        else
            g_overlay->draw_text(5, 20, D2D1::ColorF(255, 0, 0, 255), "F1 Crosshair : OFF");

        if (esp::health)
            g_overlay->draw_text(5, 40, D2D1::ColorF(0, 255, 0, 255), "F2 Health : ON");
        else
            g_overlay->draw_text(5, 40, D2D1::ColorF(255, 0, 0, 255), "F2 Health : OFF");

        if (esp::box)
            g_overlay->draw_text(5, 60, D2D1::ColorF(0, 255, 0, 255), "F4 Box : ON");
        else
            g_overlay->draw_text(5, 60, D2D1::ColorF(255, 0, 0, 255), "F4 Box : OFF");

        if (esp::distance)
            g_overlay->draw_text(5, 80, D2D1::ColorF(0, 255, 0, 255), "F5 Distance : ON");
        else
            g_overlay->draw_text(5, 80, D2D1::ColorF(255, 0, 0, 255), "F5 Distance : OFF");

        if (esp::healthT)
            g_overlay->draw_text(5, 100, D2D1::ColorF(0, 255, 0, 255), "F6 Health Text : ON");
        else
            g_overlay->draw_text(5, 100, D2D1::ColorF(255, 0, 0, 255), "F6 Health Text : OFF");
    }
}

void Update() {
    while (true)
    {
        if (rendering)
        {
            if (GetAsyncKeyState(VK_F1) & 1)
                if (showmenu)
                    esp::crosshair = !esp::crosshair;

            if (GetAsyncKeyState(VK_F2) & 1)
                if (showmenu)
                    esp::health = !esp::health;

            if (GetAsyncKeyState(VK_F4) & 1)
                if (showmenu)
                    esp::box = !esp::box;

            if (GetAsyncKeyState(VK_F5) & 1)
                if (showmenu)
                    esp::distance = !esp::distance;

            if (GetAsyncKeyState(VK_F6) & 1)
                if (showmenu)
                    esp::healthT = !esp::healthT;
        }
        if (GetAsyncKeyState(VK_INSERT) & 1)
            showmenu = !showmenu;
    }
}

static void render(FOverlay* overlay) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        overlay->begin_scene();
        overlay->clear_scene();
        frame++;
        RenderMenu();
        ESPLoop();
        overlay->end_scene();
    }
}

static void _init(FOverlay* overlay) {
    if (!overlay->window_init()) {
        printf("[!] Error init overlay window\n");
        Sleep(5000);
        return;
    }
    else {
        printf("[+] init overlay window\n");

    }

    if (!overlay->init_d2d())
        return;

    std::thread r(render, overlay);
    std::thread up(Update);

    r.join();
    up.detach();
    overlay->d2d_shutdown();
    return;
}

int main()
{
    /* @NOTE: not really useful for you this part*/
    std::string brand = xorstr_("Unnamed");

    if (!memory::initialize())
    {
        printf(xorstr_("[-] memory init failed\n"));
        system(xorstr_("pause"));
        return 1;
    }

    memory::call<void>(xorstr_("kernel32.dll"), xorstr_("SetConsoleTitleA"), xorstr_("I LOVE UC <3"));
    /* @NOTE: not really useful for you this part*/

    /* @NOTE: the driver has been removed, implement yours :)*/
    if (!driver::load())
    {
        printf(xorstr_("[-] driver init failed\n"));
        system(xorstr_("pause"));
        return 1;
    }
    /* @NOTE: the driver has been removed, implement yours :)*/

    /* @NOTE: I collect everything that's useful for what comes next.*/
    driver::detail::process = xorstr_("cs2.exe");

    while (!driver::detail::pid)
        driver::detail::pid = memory::get_pid(xorstr_("cs2.exe"));

    uint64_t base = driver::get_process_exe_base();
    if (!base)
    {
        printf(xorstr_("[-] base addr not found\n"));
        system(xorstr_("pause"));
        return 1;
    }
    std::cout << xorstr_("[+] Found CS2 Base ---> ") << xorstr_("0x") << std::hex << base << std::dec << std::endl;

    client = driver::module(L"client.dll");
    if (!client)
    {
        printf(xorstr_("[-] base addr not found\n"));
        system(xorstr_("pause"));
        return 1;
    }
    std::cout << xorstr_("[+] Found client Base ---> ") << xorstr_("0x") << std::hex << client << std::dec << std::endl;
    /* @NOTE: I collect everything that's useful for what comes next.*/

    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    HDC monitor = GetDC(hDesktop);
    int current = GetDeviceCaps(monitor, VERTRES);
    int total = GetDeviceCaps(monitor, DESKTOPVERTRES);
    ScreenCenterX = GetSystemMetrics(SM_CXSCREEN) / 2;
    ScreenCenterY = GetSystemMetrics(SM_CYSCREEN) / 2;
    screensize = { (float)GetSystemMetrics(SM_CXSCREEN),(float)GetSystemMetrics(SM_CYSCREEN),0 };
    g_overlay = { 0 };
    _init(g_overlay);
    
}
