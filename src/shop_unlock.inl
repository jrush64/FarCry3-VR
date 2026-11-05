// Shop unlock: makes the SPAS-12 available in the shop from the DLL.
// First attempt called the script command SetShopItemsUnlockAll (engine RVA 0xc7fcd0): it ran 4 times in a live session
// and the SPAS-12 still was not in the shop. That command only touches per-instance UI state (0x117e360).
// The game's real switch is its console variable g_UnlockAllShopItems: value dword at RVA 0x285b278 (registered at
// RVA 0x1b83405; the name store is the instruction at RVA 0x1b83417, into RVA 0x285b268). The shop list builder (RVA 0x107e900: cmp [var],0 / jne)
// and the per-item availability predicate (RVA 0x1082176, same compare) read it; nonzero = every item available.
// Setting it is what the console command "g_UnlockAllShopItems 1" does. A plain dword write, no code patch.
volatile LONG* g_shopUnlockOverride=nullptr; // tests: a fake variable
volatile LONG* ShopUnlockVariable()
{
    if(g_shopUnlockOverride)return g_shopUnlockOverride;
    const UINT base=reinterpret_cast<UINT>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return nullptr;
    const BYTE* var=reinterpret_cast<const BYTE*>(base+0x285b278);
    // registration stores the name pointer: mov dword ptr [var-0x10],<"g_UnlockAllShopItems">
    // mov dword ptr [0x1285b268],"g_UnlockAllShopItems" at RVA 0x1b83417 (the registration starts at 0x1b83405; the first
    // build checked there and the readers at 0x7e900 / 0x82176 instead of 0x107e900 / 0x1082176 -> "Not available").
    const BYTE* reg=reinterpret_cast<const BYTE*>(base+0x1b83417);
    bool ok=reg[0]==0xc7&&reg[1]==0x05&&*reinterpret_cast<const UINT*>(reg+2)==base+0x285b268;
    const char* name=ok?*reinterpret_cast<const char* const*>(reg+6):nullptr;
    ok=ok&&name&&!IsBadReadPtr(name,21)&&!memcmp(name,"g_UnlockAllShopItems",21);
    // readers: cmp dword ptr [var],0 at the shop list builder and the item predicate
    BYTE cmp[7]={0x83,0x3d};*reinterpret_cast<UINT*>(cmp+2)=base+0x285b278;cmp[6]=0;
    ok=ok&&!memcmp(reinterpret_cast<const BYTE*>(base+0x107e900),cmp,7)&&!memcmp(reinterpret_cast<const BYTE*>(base+0x1082176),cmp,7);
    return ok?reinterpret_cast<volatile LONG*>(const_cast<BYTE*>(var)):nullptr;
}
volatile LONG g_shopUnlockResult=0; // 0 none, 1 unlocked, 2 normal, 3 not available
bool g_shopUnlockLogged=false;
void RequestShopUnlock(bool unlock)
{
    auto* var=ShopUnlockVariable();
    if(!var){InterlockedExchange(&g_shopUnlockResult,3);Log("[SHOP] g_UnlockAllShopItems not found (engine code differs); nothing written");return;}
    InterlockedExchange(var,unlock?1:0);InterlockedExchange(&g_shopUnlockResult,unlock?1:2);
    Log("[SHOP] g_UnlockAllShopItems = %d (the game's own console switch; shop lists every item while it is 1)",unlock?1:0);
}
bool ShopUnlocked(){auto* var=ShopUnlockVariable();return var&&*var!=0;}
void TickShopUnlock(){} // kept for the request seam; the variable write needs no game-thread call
