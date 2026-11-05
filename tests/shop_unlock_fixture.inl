// Shop unlock: the INSERT button sets the game's own g_UnlockAllShopItems console variable (nothing else is written).
void TestShopUnlock()
{
 g_shopUnlockOverride=nullptr;RequestShopUnlock(true);
 Check(g_shopUnlockResult==3,"no matching engine code (test process): nothing is written, menu says not available");
 volatile LONG fake=0;g_shopUnlockOverride=&fake;
 RequestShopUnlock(true);Check(fake==1&&g_shopUnlockResult==1&&ShopUnlocked(),"Unlock all shop weapons: g_UnlockAllShopItems = 1");
 RequestShopUnlock(false);Check(fake==0&&g_shopUnlockResult==2&&!ShopUnlocked(),"Normal shop: g_UnlockAllShopItems = 0");
 ObserveMagazineReloadRequest(0,0);Check(fake==0,"the reload seam never touches it");
 g_shopUnlockOverride=nullptr;InterlockedExchange(&g_shopUnlockResult,0);
 std::puts("PASS: shop unlock: INSERT sets the game's own g_UnlockAllShopItems console variable (1 = every shop item listed, 0 = normal), signature-checked; nothing else written.");
}
