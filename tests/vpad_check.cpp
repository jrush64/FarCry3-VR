// Plugs a virtual Xbox 360 pad, presses A, and reads it back through XInput.
#include <windows.h>
#include <Xinput.h>
#include <cstdio>
#include <ViGEm/Client.h>
int main()
{
    PVIGEM_CLIENT client = vigem_alloc();
    VIGEM_ERROR e = vigem_connect(client);
    std::printf("connect 0x%08X\n", (unsigned)e);
    if (!VIGEM_SUCCESS(e)) return 1;
    PVIGEM_TARGET pad = vigem_target_x360_alloc();
    e = vigem_target_add(client, pad);
    std::printf("add 0x%08X index %lu\n", (unsigned)e, vigem_target_get_index(pad));
    if (!VIGEM_SUCCESS(e)) return 1;
    Sleep(1500);
    XUSB_REPORT r = {};
    r.wButtons = XUSB_GAMEPAD_A;
    r.sThumbLX = 20000;
    vigem_target_x360_update(client, pad, r);
    Sleep(300);
    int found = 0;
    for (DWORD i = 0; i < 4; ++i)
    {
        XINPUT_STATE s = {};
        if (XInputGetState(i, &s) == ERROR_SUCCESS)
        {
            std::printf("XInput slot %lu: buttons 0x%04X LX %d\n", i, s.Gamepad.wButtons, s.Gamepad.sThumbLX);
            if (s.Gamepad.wButtons & XINPUT_GAMEPAD_A) found = 1;
        }
    }
    vigem_target_remove(client, pad);
    vigem_target_free(pad);
    vigem_disconnect(client);
    vigem_free(client);
    std::printf(found ? "PASS: Windows sees the virtual pad with A held\n" : "FAIL\n");
    return found ? 0 : 1;
}
