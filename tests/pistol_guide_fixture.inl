
void TestPistolGuides(ID3D11Device* device,ID3D11DeviceContext* context)
{
 auto* oldDevice=g_dev;auto* oldContext=g_ctx;g_dev=device;g_ctx=context;
 auto oldSettings=g_pistolGuides;auto oldMenu=g_pistolMenu;auto oldPistol=g_pistolReload;
 const float oldSmgStrength=g_smgHintStrength230;
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"pui",0,path);g_menuSavePath=path;
 WritePrivateProfileStringW(L"PistolReload",L"Mode",L"0",path);
 WritePrivateProfileStringW(L"Reload",L"PortX",L"0.123",path);
 g_pistolGuides=PistolGuideSettings{};g_pistolMenu=PistolGuideMenu{};
 SetInsertOpen(true);Check(g_pistolMenu.open&&fc3pad::MenuOpen(),"Insert global menu opens without weapon/tracking requirement");
 fc3pad::Controllers c={};c.active=true;c.leftX=c.leftY=c.rightX=c.rightY=c.leftTrigger=c.rightTrigger=c.leftGrip=c.rightGrip=1;c.x=c.y=c.a=c.b=c.menu=true;fc3pad::Publish(c);
 Check(!fc3pad::FireOutput()&&!fc3pad::AimOutput()&&!fc3pad::ButtonsOutput(),"all controller buttons and triggers neutral while menu is open");float x=1,y=1;fc3pad::LookStick(x,y);Check(!x&&!y,"body turning blocked by menu");
 // Backend renders genuine ImGui widgets into the WARP target.
 ImGuiContext* previous=ImGui::GetCurrentContext();if(!g_menuContext)g_menuContext=ImGui::CreateContext();ImGui::SetCurrentContext(g_menuContext);
 if(!g_menuBackend)g_menuBackend=ImGui_ImplDX11_Init(g_dev,g_ctx);Check(g_menuBackend,"menu DX11 backend ready");
 auto frame=[&](){ImGui_ImplDX11_NewFrame();MenuBeginFrame(1200,900,false);};frame();frame();
 const auto initialCrop=std::vector<int>(g_menuCrop,g_menuCrop+4);
 auto drag=[&](ImVec2 start,ImVec2 end){auto& io=ImGui::GetIO();io.AddMousePosEvent(start.x,start.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMousePosEvent(end.x,end.y);frame();io.AddMouseButtonEvent(0,false);frame();};
 drag(ImVec2(g_menuCrop[0]+220.f,g_menuCrop[1]+10.f),ImVec2(g_menuCrop[0]+270.f,g_menuCrop[1]+30.f));
 Check(g_menuCrop[0]>initialCrop[0]+30&&g_menuCrop[1]>initialCrop[1]+10,"actual title-bar mouse drag moves menu");
 const int oldWidth=g_menuCrop[2],oldHeight=g_menuCrop[3];
 drag(ImVec2(g_menuCrop[0]+g_menuCrop[2]-3.f,g_menuCrop[1]+g_menuCrop[3]-3.f),ImVec2(g_menuCrop[0]+g_menuCrop[2]-130.f,g_menuCrop[1]+g_menuCrop[3]-130.f));
 Check(g_menuCrop[2]<oldWidth-60&&g_menuCrop[3]<oldHeight-60,"actual corner mouse drag resizes menu");
 const auto editedCrop=std::vector<int>(g_menuCrop,g_menuCrop+4);frame();Check(g_menuCrop[0]==editedCrop[0]&&g_menuCrop[2]==editedCrop[2],"layout is not forced back to centre each frame");
 // Restore the large test layout so all twelve slider widgets remain visible.
 ImGui::SetWindowPos("Far Cry 3 VR - Weapon guides",ImVec2(initialCrop[0],initialCrop[1]));ImGui::SetWindowSize("Far Cry 3 VR - Weapon guides",ImVec2(initialCrop[2],initialCrop[3]));frame();
 std::puts("PASS: actual mouse title-bar drag and corner resize persist across frames.");
 auto click=[&](ImVec2 pos){auto& io=ImGui::GetIO();io.AddMousePosEvent(pos.x,pos.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();};
 const bool oldActionGuide=g_reloadActionGuide227;
 click(g_actionCheckPos227);Check(g_reloadActionGuide227!=oldActionGuide&&GetPrivateProfileIntW(L"ReloadHints",L"ActionGuide",99,path)==(g_reloadActionGuide227?1:0),"227 actual mouse action-guide checkbox updates and saves independently");
 click(g_actionCheckPos227);Check(g_reloadActionGuide227==oldActionGuide,"227 actual mouse action-guide checkbox restores choice");
 g_weaponGuideShown[20]=false;click(g_menuCheckPos);Check(g_weaponGuideShown[20],"mouse checkbox enables physical guides");
 const float original=g_pistolGuides.port[0];const auto slide=g_menuSliderPos[0];auto& io=ImGui::GetIO();io.AddMousePosEvent(slide.x+40,slide.y);io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();Check(g_pistolGuides.port[0]!=original,"actual mouse slider edits guide coordinate");
 g_pistolGuides.slide[0]=.24f;click(g_menuSavePos[0]);Check(g_pistolMenu.result==1,"mouse per-slider save succeeds");Check(std::fabs(PistolSetting(path,L"PortX",-1)-g_pistolGuides.port[0])<1e-6,"single slider saved exact coordinate");Check(PistolSetting(path,L"SlideX",-1)==-1,"single slider save does not persist other unsaved edits");
 click(g_menuDefaultPos[0]);Check(g_pistolGuides.port[0]==PistolGuideDefault(0)&&g_pistolGuides.slide[0]==.24f,"Default resets only its own slider");
 g_pistolGuides.ammoPickup=1;g_pistolGuides.replacementDelay=2.75f;g_pistolGuides.catchRadius=.19f;g_pistolGuides.tossHeight=1.75f;click(g_menuSaveAllPos);Check(g_pistolMenu.result==1,"mouse Save All succeeds");const auto saved=g_pistolGuides;g_pistolGuides=PistolGuideSettings{};LoadPistolGuides(path);Check(std::fabs(g_pistolGuides.slide[0]-.24f)<1e-6,"Save All survives restart with marker bool and all coordinates");
 Check(g_pistolGuides.ammoPickup==1&&g_pistolGuides.replacementDelay==2.75f&&std::fabs(g_pistolGuides.catchRadius-.19f)<1e-6&&g_pistolGuides.tossHeight==1.75f,"pickup, delay, catch radius and toss height survive SaveAll/restart");
 Check(PistolSetting(path,L"Mode",9)==0&&GetPrivateProfileIntW(L"Machete",L"Enabled",99,path)==99,"menu saves preserve unrelated weapon/reload settings");
 wchar_t untouched[64];GetPrivateProfileStringW(L"Reload",L"PortX",L"",untouched,64,path);Check(!wcscmp(untouched,L"0.123"),"pistol saves preserve shotgun calibration");
 WritePrivateProfileStringW(L"PistolReload",L"PortX",L"nan",path);LoadPistolGuides(path);Check(std::isfinite(g_pistolGuides.port[0]),"invalid persisted coordinate rejected");
 g_pistolGuides.box=true;frame();const float oldBoxWidth=g_pistolGuides.size[0][0];
 const auto widthSlider=g_menuSliderPos[12];io.AddMousePosEvent(widthSlider.x+35,widthSlider.y);io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();
 Check(g_pistolGuides.size[0][0]!=oldBoxWidth&&g_pistolMenu.target==0,"actual box width mouse slider edits live weapon volume");
 click(g_menuSavePos[12]);Check(g_pistolMenu.result==1,"actual box width Save button succeeds");
 click(g_menuDefaultPos[12]);Check(std::fabs(g_pistolGuides.size[0][0]-PistolGuideDefault(12))<1e-6,"actual box width Default button restores dimension");
 std::puts("PASS: actual mouse box dimension slider, Default and independent Save buttons");
 const bool oldGameplayHide=g_showWeaponReloadGuides;click(g_menuHideGuidesPos);
 Check(g_showWeaponReloadGuides!=oldGameplayHide&&GetPrivateProfileIntW(L"VRGuides",L"ReloadGuides",99,path)==(g_showWeaponReloadGuides?1:0),"actual mouse gameplay-guide visibility checkbox saves independently");
 Check(g_weaponGuideShown[20],"global hide preserves per-weapon visibility selection");click(g_menuHideGuidesPos);Check(g_showWeaponReloadGuides==oldGameplayHide,"guide visibility can be restored immediately");
 std::puts("PASS: actual mouse hide-gameplay-guides checkbox persists globally and leaves calibration preview enabled.");
 const auto oldAkGuides=g_magazineGuides[2];const auto oldPistolGuides=g_magazineGuides[0];
 g_menuWeapon=1;frame();frame();Check(g_pistolMenu.open&&g_menuGuideFamily==2,"AK has independent Insert menu settings");
 const bool oldAkVisible=g_weaponGuideShown[8];click(g_menuCheckPos);Check(g_weaponGuideShown[8]!=oldAkVisible&&g_magazineGuides[0].preview==oldPistolGuides.preview,"AK preview checkbox isolated from pistol");
 click(g_menuSaveAllPos);Check(g_pistolMenu.result==1,"AK Save All succeeds");
 g_pistolGuides.port[1]=.1f;LoadPistolGuides(path);Check(std::fabs(g_pistolGuides.port[1]-oldAkGuides.port[1])<.00001,"AK calibration reloads from its own saved section");
 g_magazineGuides[2]=oldAkGuides;g_menuWeapon=0;frame();
 std::puts("PASS: AK Insert menu preview/Save All/restart isolated from existing pistol and Skorpion settings.");
 const auto skGuidesBefore=g_magazineGuides[1];const auto pistolGuidesBefore=g_magazineGuides[0];
 for(int i=0;i<sizeof(kMenuWeapons)/sizeof(*kMenuWeapons);++i)if(strcmp(kMenuWeapons[i].name,"VZ61")==0)g_menuWeapon=i;frame();frame();
 const float priorStrength=g_smgHintStrength230;auto strengthControl=g_smgStrengthPos230;click(ImVec2(strengthControl.x+50,strengthControl.y));
 Check(g_smgHintStrength230!=priorStrength,"230 actual mouse slider edits shared SMG strength on selected Skorpion page");const float savedStrength=g_smgHintStrength230;g_smgHintStrength230=1;Load1911Tint49(path);Check(std::fabs(savedStrength-g_smgHintStrength230)<.001f,"230 mouse strength adjustment autosaves");
 Check(g_menuGuideFamily==1,"actual Insert menu selects Skorpion profile");const bool skPreview=g_weaponGuideShown[39];click(g_menuCheckPos);Check(g_weaponGuideShown[39]!=skPreview&&g_magazineGuides[0].preview==pistolGuidesBefore.preview,"Skorpion mouse checkbox changes only its own markers");
 const float skPortX=g_pistolGuides.port[0];auto skSlider=g_menuSliderPos[0];io.AddMousePosEvent(skSlider.x+45,skSlider.y);io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();Check(g_pistolGuides.port[0]!=skPortX,"Skorpion mouse slider changes its own insertion guide");
 click(g_menuSaveAllPos);Check(g_pistolMenu.result==1,"actual Skorpion Save All succeeds");const float savedSkPort=g_pistolGuides.port[0];g_pistolGuides.port[0]=0;LoadPistolGuides(path);Check(std::fabs(g_pistolGuides.port[0]-savedSkPort)<1e-6,"Skorpion mouse Save All restores separate section on restart");
 g_magazineGuides[1]=skGuidesBefore;g_menuWeapon=0;frame();Check(g_menuGuideFamily==0&&g_pistolGuides.port[0]==pistolGuidesBefore.port[0],"returning from Skorpion preserves 1911 calibration");
 std::puts("PASS: actual Skorpion Insert menu mouse checkbox/slider/Save All/restart are isolated from 1911");
 // New magazine-placement preview and real mouse position/rotation edits.
 click(g_menuAmmoCheckPos);Check(g_pistolGuides.ammoPreview,"per-weapon mouse checkbox enables ammo preview");frame();
 const auto posControl=g_menuSliderPos[24];drag(posControl,ImVec2(posControl.x-35,posControl.y));
 Check(g_pistolGuides.ammoPosition[0]<0,"ammo position slider allows negative hand-local offsets");
 const auto rotControl=g_menuSliderPos[27];drag(rotControl,ImVec2(rotControl.x+20,rotControl.y));
 Check(g_pistolGuides.ammoRotation[0]!=0,"actual mouse ammo rotation slider edits degrees");
 const float chosenAmmoX=g_pistolGuides.ammoPosition[0];click(g_menuSavePos[24]);
 Check(std::fabs(PistolSetting(path,L"AmmoPosX",9)-chosenAmmoX)<1e-6,"single ammo position slider saves exact offset");
 click(g_menuDefaultPos[24]);Check(g_pistolGuides.ammoPosition[0]==0,"ammo Default resets only chosen coordinate");
 click(g_menuSaveAllPos);const float chosenAmmoRot=g_pistolGuides.ammoRotation[0];g_pistolGuides.ammoRotation[0]=0;LoadPistolGuides(path);
 Check(std::fabs(g_pistolGuides.ammoRotation[0]-chosenAmmoRot)<1e-6,"Save All restores ammo orientation on restart");
 std::puts("PASS: actual mouse ammo preview checkbox, signed translation, rotation, per-slider Default/Save and Save All restart");
 // Render actual pixels and ensure pipeline state is restored.
 D3D11_TEXTURE2D_DESC td={};td.Width=1200;td.Height=900;td.ArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* target=nullptr;Check(SUCCEEDED(g_dev->CreateTexture2D(&td,nullptr,&target)),"menu WARP target");
 ID3D11RenderTargetView* rtv=nullptr;g_dev->CreateRenderTargetView(target,nullptr,&rtv);g_ctx->OMSetRenderTargets(1,&rtv,nullptr);
 D3D11_VIEWPORT vp={3,4,1100,850,0,1};g_ctx->RSSetViewports(1,&vp);
 RenderInsertMenu(target,false);ID3D11RenderTargetView* restored=nullptr;g_ctx->OMGetRenderTargets(1,&restored,nullptr);Check(restored==rtv,"menu restores original render target");if(restored)restored->Release();D3D11_VIEWPORT after={};UINT count=1;g_ctx->RSGetViewports(&count,&after);Check(!memcmp(&vp,&after,sizeof(vp)),"menu restores viewport");
 td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;g_dev->CreateTexture2D(&td,nullptr,&stage);g_ctx->CopyResource(stage,g_menuTexture);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(g_ctx->Map(stage,0,D3D11_MAP_READ,0,&map)),"menu GPU readback");
 unsigned bright=0;for(UINT row=0;row<td.Height;++row)for(UINT col=0;col<td.Width;++col){auto* px=static_cast<BYTE*>(map.pData)+row*map.RowPitch+col*4;if(px[0]>120&&px[1]>120&&px[2]>120)++bright;}Check(bright>10000,"menu widgets visible in actual WARP pixels");
 BITMAPFILEHEADER bh={};bh.bfType=0x4d42;bh.bfOffBits=sizeof(bh)+sizeof(BITMAPINFOHEADER);bh.bfSize=bh.bfOffBits+td.Width*td.Height*4;BITMAPINFOHEADER ih={};ih.biSize=sizeof(ih);ih.biWidth=td.Width;ih.biHeight=-static_cast<LONG>(td.Height);ih.biPlanes=1;ih.biBitCount=32;FILE* f=nullptr;fopen_s(&f,"insert-menu-preview.bmp","wb");fwrite(&bh,sizeof(bh),1,f);fwrite(&ih,sizeof(ih),1,f);for(UINT row=0;row<td.Height;++row)fwrite(static_cast<BYTE*>(map.pData)+row*map.RowPitch,td.Width*4,1,f);fclose(f);g_ctx->Unmap(stage,0);stage->Release();rtv->Release();target->Release();
 SetInsertOpen(false);fc3pad::Controllers neutral={};fc3pad::Publish(neutral);Check(!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!g_pistolReload.pending&&!g_pistolReload.cycle,"close restores input without creating reload");

 // The read-only trace must keep working in game menus and preserve camera/pipeline state.
 wchar_t probe[MAX_PATH];GetTempFileNameW(folder,L"rpc",0,probe);
 g_renderProbe=CreateFileW(probe,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);g_renderProbeBytes=0;
 const auto oldCamera=g_frameCamera;const auto oldBins=g_cameraBins[0];const auto oldGameplay=g_inGameplay;
 g_inGameplay=false;g_pistolMenu.open=true;TickRenderTransitionProbe(nullptr);
 g_inGameplay=true;g_pistolMenu.open=false;TickRenderTransitionProbe(nullptr);
 Check(!memcmp(&oldCamera,&g_frameCamera,sizeof(oldCamera))&&!memcmp(&oldBins,&g_cameraBins[0],sizeof(oldBins)),"render probes do not mutate cameras/bins");
 CloseHandle(g_renderProbe);g_renderProbe=INVALID_HANDLE_VALUE;FILE* log=nullptr;_wfopen_s(&log,probe,L"rb");char bytes[16000]={};fread(bytes,1,sizeof(bytes)-1,log);fclose(log);Check(strstr(bytes,"gameplay=0,insert=1")&&strstr(bytes,"gameplay=1,insert=0"),"probe captures native game/menu and Insert transitions");DeleteFileW(probe);g_inGameplay=oldGameplay;
 std::puts("PASS: read-only rendering probes capture game/menu transitions without camera mutation.");
 g_pistolGuides=oldSettings;g_pistolMenu=oldMenu;g_pistolReload=oldPistol;g_menuSavePath=nullptr;DeleteFileW(path);g_dev=oldDevice;g_ctx=oldContext;ImGui::SetCurrentContext(previous);
 g_smgHintStrength230=oldSmgStrength;
 std::puts("PASS: mouse Insert menu, checkbox, slider edits, Default, isolated Save, Save All/restart, all pad input blocked, weapon placeholders, WARP pixels and render target/viewport restoration.");
}
