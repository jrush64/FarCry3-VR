#define FC3_AMMO_TEST 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// Deterministic sample times for physical-motion fixtures; other tests use the OS clock.
ULONGLONG g_fixtureClock171=0;
ULONGLONG FixtureClock171(){return g_fixtureClock171?g_fixtureClock171 : ::GetTickCount64();}
#define GetTickCount64 FixtureClock171
#ifdef FC3_REPRO312_SOURCE
#include "repro312_stereo.inl"
#else
#include "../src/stereo.cpp"
#endif
#include "../src/throwable_launch_probe.cpp"
#include <d3dcompiler.h>
#include <cstdlib>
void Check(bool ok,const char* what) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);} }
void STDMETHODCALLTYPE Update(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,const D3D11_BOX* box,const void* p,UINT rp,UINT dp)
{ c->UpdateSubresource(r,s,box,p,rp,dp); }
ID3D11Buffer* Buffer(ID3D11Device* d,UINT size,UINT bind)
{
    D3D11_BUFFER_DESC desc={};desc.ByteWidth=size;desc.BindFlags=bind;ID3D11Buffer* b=nullptr;
    Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&b)),"buffer creation");return b;
}
void Read(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* source,float* out)
{
    D3D11_BUFFER_DESC desc={};source->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* stage=nullptr;Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&stage)),"staging buffer");c->CopyResource(stage,source);
    D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(c->Map(stage,0,D3D11_MAP_READ,0,&m)),"read upload");std::memcpy(out,m.pData,desc.ByteWidth);c->Unmap(stage,0);stage->Release();
}
void Compare(const float* a,const float* b,int n,const char* what)
{for(int i=0;i<n;++i)Check(std::isfinite(a[i]) && std::fabs(a[i]-b[i])<2e-5f,what);}
void STDMETHODCALLTYPE ProbeCopy(ID3D11DeviceContext* c,ID3D11Resource* d,ID3D11Resource* s){c->CopyResource(d,s);}
HRESULT STDMETHODCALLTYPE ProbeMap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,D3D11_MAP t,UINT f,D3D11_MAPPED_SUBRESOURCE* m){return c->Map(r,s,t,f,m);}
void STDMETHODCALLTYPE ProbeUnmap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s){c->Unmap(r,s);}
void STDMETHODCALLTYPE EjectCopyRange(ID3D11DeviceContext* c,ID3D11Resource* d,UINT ds,UINT x,UINT y,UINT z,ID3D11Resource* s,UINT ss,const D3D11_BOX* b){c->CopySubresourceRegion(d,ds,x,y,z,s,ss,b);}
#include "vehicle_look_fixture.inl"
#include "glider257_fixture.inl"
#include "native_foot281_fixture.inl"
#include "native_action282_fixture.inl"
#include "trigger_finger262_fixture.inl"
#include "physical_ejection_fixture.inl"
#include "shell_visual_fixture.inl"
#include "ammo_fixture.inl"
#include "ammo_owner_fixture.inl"
#include "auto_ammo_fixture.inl"
#include "physical_reload_fixture.inl"
#include "animation_probe_fixture.inl"
#include "captured_hand_fixture.inl"
#include "weapons107_fixture.inl"
#include "pump_lock_fixture.inl"
#include "control_ownership_fixture.inl"
#include "reload_pose_fixture.inl"
#include "reload_count_trace_fixture.inl"
#include "one_shell_x_fixture.inl"
#include "machete_hold_fixture.inl"
#include "machete_melee_fixture.inl"
#include "machete_blade_contact_fixture.inl"
#include "machete_collider118_fixture.inl"
#include "machete_actor119_fixture.inl"
#include "machete_pawn120_fixture.inl"
#include "machete_feedback121_fixture.inl"
#include "machete_recovery123_fixture.inl"
#include "machete_hold_mode_fixture.inl"
#include "machete_native_fixture.inl"
#include "machete_routing_fixture.inl"
#include "machete_quick_fixture.inl"
 #include "view_ownership_fixture.inl"
#include "machete_ownership_fixture.inl"
#include "machete_swept_fixture.inl"
#include "reload_signal_fixture.inl"
#include "manual1911_rack50.inl"
#include "pistol_reload_fixture.inl"
#include "pistol_mechanics_fixture.inl"
#include "pistol_guide_fixture.inl"
#include "pistol_preview_fixture.inl"
#include "pistol_box_fixture.inl"
#include "skorpion_fixture.inl"
#include "skorpion_charge52.inl"
#include "r700_calibration53.inl"
#include "bow_sight54.inl"
#include "bow_sight55.inl"
#include "bow58.inl"
#include "flamer_support58.inl"
#include "muzzle_render58.inl"
#include "scope60.inl"
#include "scope_clean61.inl"
#include "scope_projection62.inl"
#include "scope_controls63.inl"
#include "scope_fusion64.inl"
#include "scope_followup65.inl"
#include "diagnostic66.inl"
#include "scope67.inl"
#include "scope72.inl"
#include "reticle73.inl"
#include "arms77.inl"
#include "visibility68.inl"
#include "lever1887_calibration53.inl"
#include "reload_calibration54.inl"
#include "flamer_calibration55.inl"
#include "shotgun_support56.inl"
#include "lever1887_palm56.inl"
#include "m133_pose57.inl"
#include "m133_sight57.inl"
#include "ak_fixture.inl"
#include "p6p9_fixture.inl"
#include "rifles_fixture.inl"
#include "lever1887_fixture.inl"
#include "lever1887_shells_fixture.inl"
#include "lever1887_simple_fixture.inl"
#include "shop_unlock_fixture.inl"
#include "revolver_fixture.inl"
#include "revolver_assembly217.inl"
#include "heal_probe217.inl"
#include "native_shot219.inl"
#include "flare_fixture.inl"
#include "rpg_fixture.inl"
#include "gm94_fixture.inl"
#include "mechanism_interruptions219.inl"
#include "flamer_fixture.inl"
#include "spas_fixture.inl"
#include "reload_recovery221.inl"
#include "fire_ownership222.inl"
#include "clip_pause223.inl"
#include "magazine_recovery224.inl"
#include "d50_fixture.inl"
#include "ammo_preview_persistence_fixture.inl"
#include "weapons7_guides_fixture.inl"
#include "weapons8_capture_fixture.inl"
#include "repair_tool_fixture.inl"
#include "bow_fixture.inl"
#include "bow_reload_fixture.inl"
#include "gameplay_assist_fixture.inl"
#include "controller_grip124_fixture.inl"
#include "shotgun_aim125_fixture.inl"
#include "machete_world126_fixture.inl"
#include "camera_ownership127_fixture.inl"
#include "shooting129_fixture.inl"
#include "holographic131_fixture.inl"
#include "d50_capture133_fixture.inl"
#include "dot135_fixture.inl"
#include "calibration136_fixture.inl"
#include "mp5_root142_fixture.inl"
#include "weapon_bindings218.inl"
#include "weapon_attachments220.inl"
#include "grenade_pin154_fixture.inl"
#include "throwables144_fixture.inl"
#include "throw_camera153_fixture.inl"
#include "hand_ui_fixture.inl"
#include "night_floor_fixture.inl"
#include "xr_copy174_fixture.inl"
#include "render_ownership174_fixture.inl"
#include "handle_palm175_fixture.inl"
#include "trigger_parent41.inl"
#include "guide_groups42.inl"
#include "guides44.inl"
#include "heal_probe215.inl"
#include "pistol_precision43.inl"
#include "pistol_frame45.inl"
#include "pistol_order46.inl"
#include "pistol_basis47.inl"
#include "pistol_native_guide48.inl"
#include "pistol_slide_tint49.inl"
#include "weapon_native_guides50.inl"
#include "weapon_sight_pose51.inl"
#include "bow_native_guides51.inl"
#include "flamer_pose51.inl"
#include "weapon_guides_bridge51.inl"
#include "d50_sound225_fixture.inl"
#include "d50_seat226_fixture.inl"
#include "action_cost228_fixture.inl"
#include "smg229_fixture.inl"
#include "skorpion_insert230_fixture.inl"
#include "1911_seat313_fixture.inl"
#include "6p9_seat315_fixture.inl"
#include "shotgun231_fixture.inl"
#include "assault234_fixture.inl"
#include "rifle_casing235_fixture.inl"
#include "svd_lens236_fixture.inl"
#include "support_contact236_fixture.inl"
#include "special239_fixture.inl"
#include "special247_fixture.inl"
#include "special248_fixture.inl"
#include "body_runtime285_fixture.inl"
#include "hands_only287_fixture.inl"
#include "magazine_pull286_fixture.inl"
int main()
{
    // Exercise the real presenter settings API without touching the game INI.
    const wchar_t* settingsPath297=L"evidence/holsters297-settings-test.ini";
    DeleteFileW(settingsPath297);
    WritePrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"unchanged",settingsPath297);
    Check(fc3xr::WeaponHolsterSettings297().mode==fc3holster297::Four,"body holsters default to four slots without an A opener");
    for(int mode=0;mode<3;++mode){fc3holster297::Settings settings;settings.mode=mode;settings.height=-.1f;settings.radius=.15f;
      Check(fc3xr::ConfigureWeaponHolsters297(settings,settingsPath297),"all holster modes save through production API");
      Check(GetPrivateProfileIntW(L"WeaponHolsters",L"Mode",99,settingsPath297)==mode&&fc3xr::WeaponHolsterSettings297().mode==mode,"saved mode matches active mode");
      wchar_t value[64];GetPrivateProfileStringW(L"WeaponHolsters",L"HeightOffset",L"invalid",value,64,settingsPath297);Check(std::fabs(wcstod(value,nullptr)+.1)<.00001,"height setting persists");
      GetPrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"",value,64,settingsPath297);Check(wcscmp(value,L"unchanged")==0,"holster save preserves existing reload settings");
    }
    auto invalid297=fc3xr::WeaponHolsterSettings297();invalid297.mode=99;Check(!fc3xr::ConfigureWeaponHolsters297(invalid297,settingsPath297),"invalid mode cannot replace working mode");invalid297.mode=0;invalid297.radius=.001f;Check(!fc3xr::ConfigureWeaponHolsters297(invalid297,settingsPath297),"invalid radius rejected");
    std::puts("PASS:297 default, mode persistence, bounds and reload-settings preservation");

    if(GetEnvironmentVariableW(L"FC3_MAGPULL286_ONLY",nullptr,0)){TestMagazinePull286();return 0;}
    if(GetEnvironmentVariableW(L"FC3_BODY285_ONLY",nullptr,0)){TestBodyRuntime285();return 0;}
    if(GetEnvironmentVariableW(L"FC3_COLLIDER_ABI_ONLY",nullptr,0)){MH_Initialize();TestMacheteCollider118();TestMacheteActors119();TestMachetePawn120();TestMacheteFeedback121();TestMacheteRecovery123();TestMacheteWorld126();TestFastTurnCapture126();return 0;}
    if(GetEnvironmentVariableW(L"FC3_GUIDE_GROUPS_ONLY",nullptr,0)){TestGuideGroups42();TestGuides44();return 0;}
    if(GetEnvironmentVariableW(L"FC3_HEAL215_ONLY",nullptr,0)){TestHealProbe215();return 0;}
    TestHealProbe215();
    if(GetEnvironmentVariableW(L"FC3_TRIGGER_ONLY",nullptr,0)){TestTriggerParenting41();return 0;}
    TestTriggerParenting41();
    if(GetEnvironmentVariableW(L"FC3_RELOAD_SIGNAL_ONLY",nullptr,0)){TestReloadSignal();return 0;}
    {
      const auto mag=g_magazineStates[0];const auto lever=g_lever;const auto spas=g_spas;const auto rpg=g_rpg;const auto flare=g_flare;
      g_magazineStates[0].deadline=1500;g_magazineStates[0].looseAt=900;g_magazineStates[0].inputPauseAt=800;g_magazineStates[0].magazinePresent=false;g_magazineStates[0].held=true;
      g_lever.deadline=1200;g_lever.clipAt=900;g_spas.clipAt=0;
      g_rpg.clipAt=900;g_rpg.inputPauseAt219=950;g_rpg.shot.until=g_flare.shot.until=1200;
      fc3stereo::ResumeNativeWeaponTimers(1000,60000);
      Check(g_magazineStates[0].deadline==61500&&g_magazineStates[0].looseAt==60900&&g_magazineStates[0].inputPauseAt==60800,"native menu does not expire magazine insertion/falling/tracking-pause timers");
      Check(g_rpg.clipAt==60900&&g_rpg.inputPauseAt219==60950&&g_rpg.shot.until==61200&&g_flare.shot.until==61200,"219 native menu preserves nested tracking pause and pending shot windows");
      Check(!g_magazineStates[0].magazinePresent&&g_magazineStates[0].held&&g_lever.deadline==61200&&g_lever.clipAt==60900&&g_spas.clipAt==0,"native menu preserves held reload state, shell/clip timing and inactive clocks");
      g_magazineStates[0]=mag;g_lever=lever;g_spas=spas;g_rpg=rpg;g_flare=flare;
      puts("PASS: native menu pause preserves physical magazine ownership and insertion/animation timing");
    }
    g_weaponMotionGuard=false; // active-path fixtures; guard tested explicitly below
    setvbuf(stdout,nullptr,_IONBF,0);
    g_manualReloadEnabled=false; // legacy fixtures keep the accepted passthrough policies
    g_singleShellEnabled=false; // legacy/passthrough coverage; enabled explicitly by one-shell fixture
    wchar_t path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d11.dll");
    auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(path),"D3D11CreateDevice"));
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;
    Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP");
    if(GetEnvironmentVariableW(L"FC3_HANDS287_ONLY",nullptr,0)){TestHandsOnly287(device,context);context->ClearState();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_CALIBRATION54_ONLY",nullptr,0)){TestReloadCalibration54();TestFlamerCalibration55();TestLever1887Calibration53();TestBowSight54(device,context);TestBowSight55(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_ARMS77_ONLY",nullptr,0)){TestArmIK77();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_R70053_ONLY",nullptr,0)){TestR700Calibration53();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_FLAMER51_ONLY",nullptr,0)){TestFlamerPose51(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SIGHTS51_ONLY",nullptr,0)){TestWeaponSightPose51(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_BOW51_ONLY",nullptr,0)){TestBowNativeGuides51(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_BRIDGE51_ONLY",nullptr,0)){TestBodyGuideBridge51(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_RACK50_ONLY",nullptr,0)){TestManual1911Rack50();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_GUIDES50_ONLY",nullptr,0)){TestNativeGuide48(device,context);TestWeaponGuides50(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_TINT_ONLY",nullptr,0)){TestSlideTint49(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_NATIVE_GUIDE_ONLY",nullptr,0)){TestNativeGuide48(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_1911_ONLY",nullptr,0)){TestCameraTrace47(device);TestPistolBasis47(device,context);TestPistolOrder46(device,context);TestPistolTrace46(device,context);TestPistolFrame45(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_FRAME_ONLY",nullptr,0)){TestPistolFrame45(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_PRECISION_ONLY",nullptr,0)){TestPistolPrecision43(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_NIGHT_ONLY",nullptr,0)){TestNightFloor(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_HANDUI_ONLY",nullptr,0)){TestHandUi(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_ASSIST_ONLY",nullptr,0)){TestGameplayAssist(device,context);context->Release();device->Release();return 0;}
    for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_eyeViewsBody[side].view[i]=(i%5==0)?1.f:0.f;
    g_oUpdateSubresource=Update;g_motionGun=true;g_eyeViewsSet=true;g_handLeftValid=g_handRightValid=true;
    g_visibleWeaponDrawsLastFrame=0;g_twoHand=false;
    for(int side=0;side<2;++side)
    {
        float* pose=side==0?g_handRight:g_handLeft;std::memset(pose,0,16*sizeof(float));
        pose[0]=pose[5]=pose[10]=pose[15]=1;pose[3]=side==0?.3f:-.3f;pose[7]=-.2f;pose[11]=.45f;
        std::memset(g_handRef[side],0,16*sizeof(float));g_handRef[side][0]=g_handRef[side][5]=g_handRef[side][10]=g_handRef[side][15]=1;
        memcpy(g_handBody[side],pose,16*sizeof(float));
        ArmJoints j={{side==0?.2:-.2,-.15,-.2},{side==0?.35:-.35,-.3,.05},{side==0?.35:-.35,-.2,.4},true};g_armJoints[side]=j;
        g_armPaletteCb[side]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    }
    if(GetEnvironmentVariableW(L"FC3_COPY174_ONLY",nullptr,0)){TestXrCopy174(device,context);TestRenderOwnership174();return 0;}
    if(GetEnvironmentVariableW(L"FC3_HANDLES175_ONLY",nullptr,0)){TestHandlePalm175();TestSkorpionCharge52();return 0;}
    if(GetEnvironmentVariableW(L"FC3_THROW_CAMERA153_ONLY",nullptr,0)){TestThrowCamera153(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_THROWABLES_144_ONLY",nullptr,0)){TestThrowables144(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_MP5_142_ONLY",nullptr,0)){for(int profile:{28,21,25,43})TestMp5Root142(device,context,profile);return 0;}
    if(GetEnvironmentVariableW(L"FC3_CALIBRATION136_ONLY",nullptr,0)){TestCalibration136(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_AIMINGDOT134_ONLY",nullptr,0)){TestDotModes135(device,context);TestScopeFallback135(device,context);TestPistolDotFallback135(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_OPTICS131_ONLY",nullptr,0)){TestDotModes135(device,context);TestScopeFallback135(device,context);TestPistolDotFallback135(device,context);TestPhysicalScope131(device,context);TestScopes60(device,context);TestScopeProjection62(device,context);TestScopeControls63(device,context);TestScopeFollowup65(device,context);TestScope67(device,context);TestScope72(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SHOOTING129_ONLY",nullptr,0)){TestControllerGrip124(device,context);TestShooting129(device,context);TestScopes60(device,context);TestScopeRay129(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_M133233_ONLY",nullptr,0)){g_oCopyResource=ProbeCopy;g_oCopySubresourceRegion=EjectCopyRange;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;TestReloadShellVisual(device,context);TestReloadPoseLifecycle(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_RELOAD124_ONLY",nullptr,0)){TestReloadPoseLifecycle(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SHOTGUN125_ONLY",nullptr,0)){TestControllerGrip124(device,context);TestShotgunAim125(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SIGNATURE250_ONLY",nullptr,0)){TestSpecial248(device,context);TestRifleCasings235(device,context);TestOneShellX(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SPECIAL248_ONLY",nullptr,0)){TestSpecial248(device,context);TestSpecial247(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SPECIAL247_ONLY",nullptr,0)){TestSpecial247(device,context);TestRevolverAssembly217(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_ASSEMBLY217_ONLY",nullptr,0)){TestRevolverAssembly217(device,context);TestHealProbe217(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_BINDINGS218_ONLY",nullptr,0)){TestWeaponPaletteBindings218(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_ATTACHMENTS220_ONLY",nullptr,0)){TestWeaponAttachments220(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SNIPER236_ONLY",nullptr,0)){TestSupportContact236();TestSvdLens236(device,context);assault234::Test(device,context);TestRifleCasings235(device,context);TestR700Calibration53();return 0;}
    if(GetEnvironmentVariableW(L"FC3_CASINGS235_ONLY",nullptr,0)){TestRifleCasings235(device,context);TestShotgun231(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_ASSAULT234_ONLY",nullptr,0)){assault234::Test(device,context);smg229test::Test(device,context);TestActionCost228(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SMG229_ONLY",nullptr,0)){TestShotgun231(device,context);skseat230::Test();smg229test::Test(device,context);TestHandlePalm175();TestSkorpionCharge52();return 0;}
    if(GetEnvironmentVariableW(L"FC3_COST228_ONLY",nullptr,0)){TestActionCost228(device,context);TestSlideTint49(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_SEAT226_ONLY",nullptr,0)){seat226test::TestD50Seat226();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SOUND225_ONLY",nullptr,0)){sound225test::TestD50Sound225();return 0;}
    if(GetEnvironmentVariableW(L"FC3_MAGAZINES224_ONLY",nullptr,0)){TestMagazineRecovery224();return 0;}
    if(GetEnvironmentVariableW(L"FC3_CLIPS223_ONLY",nullptr,0)){TestClipPause223();return 0;}
    if(GetEnvironmentVariableW(L"FC3_FIRE222_ONLY",nullptr,0)){TestFireOwnership222();TestGm94();return 0;}
    if(GetEnvironmentVariableW(L"FC3_RECOVERY221_ONLY",nullptr,0)){TestShellRecovery221();TestBoltRegrab221();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SPECIAL239_ONLY",nullptr,0)){TestSpecial239(device,context);TestNativeSingleShots219();TestFlare();TestRpg();TestFlamer();TestFlamerPose51(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_WEAPONS219_ONLY",nullptr,0)){TestNativeSingleShots219();TestFlare();TestRpg();TestGm94();TestFlamer();TestMechanismInterruptions219();return 0;}
    if(GetEnvironmentVariableW(L"FC3_CONTROLLER124_ONLY",nullptr,0)){TestControllerGrip124(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_VISIBILITY68_ONLY",nullptr,0)){TestCameraOwnership127();TestVisibility68(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_PROBE66_ONLY",nullptr,0)){TestDiagnostic66(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SCOPE60_ONLY",nullptr,0)){TestScopes60(device,context);TestScopeProjection62(device,context);TestCleanScope61(device,context);TestScopeControls63(device,context);TestScopeFusion64();TestScopeFollowup65(device,context);TestScope67(device,context);TestScope72(device,context);TestReticle73();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_WEAPON58_ONLY",nullptr,0)){TestLever1887Palm56();TestBowSight58(device,context);TestBowString58();TestFlamerSupport58();TestMuzzleRender58(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_M13357_ONLY",nullptr,0)){TestM133Pose57(device,context);TestM133Sight57(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SHOTGUN56_ONLY",nullptr,0)){TestLever1887Palm56();TestShotgunSupport56();TestLever1887Calibration53();TestFlamerCalibration55();context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_SMALL52_ONLY",nullptr,0)){TestSkorpionCharge52();TestSkorpion(device,context);TestGameplayAssist(device,context);context->Release();device->Release();return 0;}
    if(GetEnvironmentVariableW(L"FC3_NATIVE281_ONLY",nullptr,0)){TestNativeFoot281();return 0;}
    if(GetEnvironmentVariableW(L"FC3_VEHICLE170_ONLY",nullptr,0)){TestVehicleWheel187();TestVehicleEpisode170();TestVehicleLook();TestGlider257();TestNativeFoot281();return 0;}
    if(GetEnvironmentVariableW(L"FC3_CAPTURE169_ONLY",nullptr,0)){g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;TestAnimationBatch169();TestAnimationTimeline(device,context);return 0;}
    if(GetEnvironmentVariableW(L"FC3_D50_168_ONLY",nullptr,0)){TestD50Transfer168();return 0;}
    if(GetEnvironmentVariableW(L"FC3_BOW168_ONLY",nullptr,0)){g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;TestBow(device,context);TestBowReload();return 0;}
    if(GetEnvironmentVariableW(L"FC3_PISTOL_ONLY",nullptr,0)){TestPistolReload(device,context);TestPistolMechanics(device,context);TestSkorpion(device,context);TestPistolGuides(device,context);TestPistolPreview(device,context);TestPistolBoxes();TestPersistentAmmoPreview(device,context);TestAk(device,context);TestP6p9(device,context);TestRifles(device,context);TestLever1887(device,context);TestLever1887Shells();TestLever1887Simple();TestShopUnlock();TestRevolver();TestFlare();TestRpg();TestGm94();TestFlamer();TestSpas();TestD50(device,context);TestWeapons7Guides();TestWeapons8Capture(device,context);g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;TestRepairIndexReadback(device,context);TestPistolEarlyPiece();TestPistolAttachmentAndSupport();TestRepairToolEffects();TestBow(device,context);TestBowReload();context->ClearState();context->Release();device->Release();return 0;}
    g_bodyPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
    ID3D11Buffer* cb[4]={Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),nullptr,Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER)};
    float globals[88]={};globals[0]=globals[5]=globals[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);
    CameraSummary cam={};cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.mainView=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    context->VSSetConstantBuffers(0,4,cb);
    float original[576]={};
    for(int b=0;b<48;++b){original[b*12]=original[b*12+5]=original[b*12+10]=1;original[b*12+3]=b*.001f;original[b*12+7]=b*.002f;original[b*12+11]=-b*.003f;}
    cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
    g_weaponMotionGuard=true;g_visibleWeaponDrawsLastFrame=0;
    const UINT startupEpoch=g_armPoseEpoch;
    Check(!MotionInteractionEnabled(),"restricted area pauses motion without changing saved preference");
    Check(BuildArmPalette(context,0)==nullptr&&BuildArmPalette(context,1)==nullptr,
        "safe-room arm palettes retain native rendering");
    Check(BuildBodyPalette(context)==nullptr,"safe-room sleeve/body motion override disabled");
    Mat4 pausedMove={};Check(!GunMove(g_armJoints[0],g_armJoints[1],pausedMove),"no safe-room gun transform");
    Check(g_armPoseEpoch==startupEpoch&&g_motionGun,"safe-room guard does not recalibrate or disable saved preference");
    fc3pad::Controllers hiddenControls={};hiddenControls.active=true;hiddenControls.rightTrigger=1;
    fc3pad::Publish(hiddenControls);const bool hiddenChamber=g_chambered;const LONG hiddenShot=g_lastShotFrame;
    UpdatePump(g_armJoints[0],g_armJoints[1]);
    Check(g_chambered==hiddenChamber&&g_lastShotFrame==hiddenShot,"hidden-weapon trigger cannot consume chamber or generate shot");
    hiddenControls.rightTrigger=0;fc3pad::Publish(hiddenControls);

    g_visibleWeaponDrawsLastFrame=6;
    g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
    Check(MotionInteractionEnabled()&&BuildArmPalette(context,0)!=nullptr,
        "native weapon appearance resumes accepted armed-right path");
    g_visibleWeaponDrawsLastFrame=0;
    Check(BuildArmPalette(context,0)==nullptr,"weapon disappearance pauses again");
    g_weaponMotionGuard=false;
    std::puts("PASS: restricted-area guard leaves native arms/body untouched, preserves preference and resumes on native weapon appearance");
    { // Reported (SPAS): the neutral pose was captured mid equip animation -> twisted left hand all session. A moving game arm never defines it.
     const LONG keepFrame=g_frame;g_frame=5000;float moving[576];std::memcpy(moving,original,sizeof(moving));
     // Reported again (the arm was wrong for every weapon): the game now uses ONE fixed straight pose, never a learned one
     for(int i=0;i<4;++i){moving[2*12+3]+=.02f;cb[3]->SetPrivateData(kCbCopyGuid,sizeof(moving),moving);++g_frame;Check(BuildArmPalette(context,1)!=nullptr,"fixed neutral pose: available at once, even while the game arm moves");}
     cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
     g_frame=keepFrame;}
    g_armIk84=false; // preserve legacy arm-path tests as a negative-control baseline (candidate84)
    g_neutralQuietGate=false;float baseline[2][576];
    for(int side=0;side<2;++side){Check(BuildArmPalette(context,side)!=nullptr,"neutral arm draw");Read(device,context,g_armPaletteCb[side],baseline[side]);}
    for(int frame=0;frame<60;++frame)
    {
        g_frame=frame;float animation[576];std::memcpy(animation,original,sizeof(animation));
        for(int b=0;b<25;++b)
        {
            Mat4 m=BoneMatrix(original,b), turn={};const double a=.2*std::sin(frame*.2+b*.15);
            turn.m[0]=turn.m[5]=std::cos(a);turn.m[1]=-std::sin(a);turn.m[4]=std::sin(a);turn.m[10]=turn.m[15]=1;
            turn.m[3]=.1*std::sin(frame*.3);turn.m[7]=.09*std::cos(frame*.4);turn.m[11]=.08*std::sin(frame*.2+b*.1);
            StoreBone(animation,b,Multiply(turn,m));
        }
        cb[3]->SetPrivateData(kCbCopyGuid,sizeof(animation),animation);
        for(int side=0;side<2;++side)
        {
            g_armJoints[side].e[1]=-.3+.15*std::sin(frame*.2);g_armJoints[side].w[2]=.4+.1*std::cos(frame*.3);
            Check(BuildArmPalette(context,side)!=nullptr,"animated arm draw");float out[576];Read(device,context,g_armPaletteCb[side],out);
            Compare(out,baseline[side],300,"pumping/running palettes must not move controller-driven arms");
        }
        // Current animated bones are used to identify sleeves, but receive the
        // exact final arm correction, including the removed source animation.
        // 10-08 arm rewrite (deterministic arms): only the body bones that ARE arm bones are replaced - Jason's mesh: body slots
        // 7/8/9 = left arm bones 0/22/23, 11/12/13 = right arm 0/22/23 - by that arm's FINAL bones (upper-arm group: minus the
        // rigid arm's slide, so the shirt keeps the shoulder). Every other body bone keeps the game's own animation.
        Check(BuildBodyPalette(context)!=nullptr,"sleeve palette");float sleeves[576];Read(device,context,g_bodyPaletteCb,sleeves);
        {const int seed[6][3]={{7,1,0},{8,1,22},{9,1,23},{11,0,0},{12,0,22},{13,0,23}};
         for(const auto& s:seed){const float* got=sleeves+s[0]*12;const float* arm=baseline[s[1]]+s[2]*12;const V3 slide=g_armSlide[s[1]];
          const float off[12]={0,0,0,static_cast<float>(slide.x),0,0,0,static_cast<float>(slide.y),0,0,0,static_cast<float>(slide.z)};
          for(int k=0;k<12;++k)Check(std::fabs(got[k]-(arm[k]-off[k]))<1e-3f,"sleeve bones = that arm's final bones (shirt keeps the shoulder: no slide)");}
         for(int b=0;b<48;++b){bool mapped=false;for(const auto& s:seed)mapped=mapped||s[0]==b;if(mapped)continue;
          for(int k=0;k<12;++k)Check(std::fabs(sleeves[b*12+k]-animation[b*12+k])<1e-5f,"unmapped body bones keep the game's animation");}}
    }
    { // R700 (reported over several tests: the forearm animated one-handed, the right hand/wrist in the left-hand hold): after a shot
      // the GAME's right arm works the bolt. Animate the game's right arm (bones + elbow / wrist joints) and require the drawn R700
      // right arm to stay identical, one-handed and with the gun in the left hand. Control: the AK path (game arm bones) must show it.
     const LONG keepProfile=g_weaponMechanicProfile,keepMech=g_weaponMechanicFrame,keepFrame=g_frame,keepWeaponDraws=g_visibleWeaponDrawsLastFrame;
     const auto keepR=g_r700;const ArmJoints keepJoints=g_armJoints[0],keepLeftJoints=g_armJoints[1];static PistolReloadState keepStates[16];memcpy(keepStates,g_magazineStates,sizeof(keepStates));
     const bool keepTwo=g_twoHand,keepGunValid=g_frameGunValid;const Mat4 keepGunMove=g_frameGunMove;float keepBody[2][16];memcpy(keepBody,g_handBody,sizeof(keepBody));
     memcpy(g_handBody[0],g_handRight,sizeof(g_handBody[0]));memcpy(g_handBody[1],g_handLeft,sizeof(g_handBody[1]));
     g_visibleWeaponDrawsLastFrame=6;g_twoHand=false;
     auto animateGameArm=[&](int frame){
      float animation[576];std::memcpy(animation,original,sizeof(animation));
      for(int b=0;b<25;++b){Mat4 m=BoneMatrix(original,b),turn={};const double a=.35*std::sin(frame*.3+b*.15);
       turn.m[0]=turn.m[5]=std::cos(a);turn.m[1]=-std::sin(a);turn.m[4]=std::sin(a);turn.m[10]=turn.m[15]=1;
       turn.m[3]=.12*std::sin(frame*.3);turn.m[7]=.1*std::cos(frame*.4);turn.m[11]=.09*std::sin(frame*.2+b*.1);StoreBone(animation,b,Multiply(turn,m));}
      cb[3]->SetPrivateData(kCbCopyGuid,sizeof(animation),animation);
      g_armJoints[0]=keepJoints;g_armJoints[0].e[1]+=.12*std::sin(frame*.25);g_armJoints[0].e[0]+=.06*std::cos(frame*.3);g_armJoints[0].w[2]+=.05*std::sin(frame*.35);
      g_armJoints[1]=keepLeftJoints;g_armJoints[1].e[1]+=.12*std::sin(frame*.27);g_armJoints[1].e[0]-=.06*std::cos(frame*.31);g_armJoints[1].w[2]+=.05*std::sin(frame*.33);};
     // modes: 0 R700 one-handed, 1 R700 gun in the left hand, 2 R700 two-handed, 3 R700 Simple bolt clip (two-handed),
     //        4 control: AK two-handed (game arm bones on both arms)
     const char* names[5]={"R700 one-handed","R700 gun in the left hand","R700 two-handed","R700 Simple bolt clip","AK two-handed control"};
     for(const LONG gun:{2L,23L})for(int mode=0;mode<5;++mode)
     { // the whole proof runs for the R700 and the Z93 (same bolt-action family); the AK control once
      if(gun==23&&mode==4)continue;
      g_r700=R700{};float ref[2][576]={};bool changed[2]={false,false};
      g_twoHand=mode>=2;
      for(int frame=0;frame<40;++frame)
      {
       g_frame=20000+(gun==23?1000:0)+mode*100+frame;g_weaponMechanicProfile=mode==4?8:gun;g_weaponMechanicFrame=g_frame;animateGameArm(frame);
       auto& rp=g_pistolReload;Mat4 root={};
       if(mode==1){if(frame==0){rp.rootBody=HandIdentity();R700BeginHold();}rp.rootBody=R700HoldBody();}
       else if(mode!=4&&R700ControllerRoot(root))rp.rootBody=root;
       if(mode==3){g_r700.physical=false;g_r700.chambered=false;g_r700.clip=0;g_r700.clipAt=GetTickCount64();}
       rp.rootFrame=g_frame;rp.rootEpoch=g_armPoseEpoch;
       g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
       for(int side=0;side<2;++side)
       {
        Check(BuildArmPalette(context,side)!=nullptr,"R700 arm draw");float out[576];Read(device,context,g_armPaletteCb[side],out);
        if(frame==1)std::memcpy(ref[side],out,sizeof(ref[side]));
        else if(frame>1)
        {
         if(mode==4){for(int i=0;i<300&&!changed[side];++i)changed[side]=std::fabs(out[i]-ref[side][i])>1e-3f;}
         else if(!(mode==3&&side==0)){char what[160];_snprintf_s(what,sizeof(what),_TRUNCATE,"%s%s: the game's arm animation never moves the drawn %s arm",gun==23?"[Z93] ":"",names[mode],side?"LEFT":"right");Compare(out,ref[side],300,what);}
        }
       }
      }
      if(mode==4)Check(!changed[0]&&changed[1],"controller124: AK right hand rejects native animation while native left grip fingers remain available");
     }
     g_r700=keepR;g_armJoints[0]=keepJoints;g_armJoints[1]=keepLeftJoints;memcpy(g_handBody,keepBody,sizeof(keepBody));g_weaponMechanicProfile=keepProfile;g_weaponMechanicFrame=keepMech;g_frame=keepFrame;g_visibleWeaponDrawsLastFrame=keepWeaponDraws;
     cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);memcpy(g_magazineStates,keepStates,sizeof(keepStates));g_twoHand=keepTwo;g_frameGunValid=keepGunValid;g_frameGunMove=keepGunMove;
     std::puts("PASS: R700 arms: with the game's arms animated (bones + elbow/wrist joints), BOTH drawn arms are identical frame to frame one-handed, two-handed, with the gun in the left hand and during the Simple bolt clip (left), for the R700 AND the Z93; the AK right hand rejects native animation while left grip fingers retain their pose");
    }
    // Body/camera rotations must rotate the stable result once, not leave it
    // pinned in the old camera axes or apply the body rotation twice.
    g_aimActive=true;const double yaw=.6;g_aimQ[0][0]=g_aimQ[2][2]=std::cos(yaw);g_aimQ[0][2]=std::sin(yaw);g_aimQ[2][0]=-std::sin(yaw);g_aimQ[1][1]=1;
    const Mat4 q=ArmBodyToCamera();float controller[16];std::memcpy(controller,g_handLeft,sizeof(controller));Mat4 cp={};for(int i=0;i<16;++i)cp.m[i]=controller[i];cp=Multiply(q,cp);for(int i=0;i<16;++i)g_handLeft[i]=static_cast<float>(cp.m[i]);
    Check(BuildArmPalette(context,1)!=nullptr,"body-rotated stable arm");float turned[576];Read(device,context,g_armPaletteCb[1],turned);
    float expected[576];std::memcpy(expected,baseline[1],sizeof(expected));for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(q,BoneMatrix(baseline[1],b)));
    Compare(turned,expected,300,"body rotation applied exactly once");
    g_aimActive=false;std::memcpy(g_handLeft,controller,sizeof(controller));
    // With a visible weapon, the right hand keeps the confirmed existing path.
    g_visibleWeaponDrawsLastFrame=6;g_frameGunValid=true;g_frameGunMove={};g_frameGunMove.m[0]=g_frameGunMove.m[5]=g_frameGunMove.m[10]=g_frameGunMove.m[15]=1;
    cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
    Mat4 hand=g_frameGunMove,fore={},upper={};Check(SolveArm(g_armJoints[0],hand,0,fore,upper,true),"armed solve");
    const Mat4 moves[3]={upper,fore,hand};std::memcpy(expected,original,sizeof(expected));for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(moves[ArmGroup(b)],BoneMatrix(original,b)));
    Check(BuildArmPalette(context,0)!=nullptr,"armed right hand");Read(device,context,g_armPaletteCb[0],turned);{Mat4 target124={};Check(ControllerRightHand124(target124),"armed right controller target available");for(int i=0;i<12;++i)Check(fabs(turned[24+i]-target124.m[i])<1e-5,"armed right wrist follows the controller-owned target rather than the former native attachment");}
    // While holding, preserve the original grip pose and physical slide path.
    g_twoHand=true;g_slideGrab=.1;g_slide=.06;g_slideAxis[0]=0;g_slideAxis[1]=0;g_slideAxis[2]=1;
    g_pumpAnimView[0]=0;g_pumpAnimView[1]=0;g_pumpAnimView[2]=-.09;
    Mat4 grip={};Check(LeftMove(g_armJoints[1],g_armJoints[0],grip),"left grip move");
    Check(SolveArm(g_armJoints[1],grip,1,fore,upper,true),"left grip solve");
    const Mat4 gripMoves[3]={upper,fore,grip};std::memcpy(expected,original,sizeof(expected));
    for(int b=0;b<25;++b)StoreBone(expected,b,Multiply(gripMoves[ArmGroup(b)],BoneMatrix(original,b)));
    Check(BuildArmPalette(context,1)!=nullptr,"held left hand");Read(device,context,g_armPaletteCb[1],turned);
    Compare(turned,expected,300,"held left hand retains game grip pose and pump-slide correction");
    // Releasing immediately restores the stable free-hand pose.
    g_twoHand=false;Check(BuildArmPalette(context,1)!=nullptr,"released left hand");Read(device,context,g_armPaletteCb[1],turned);
    Compare(turned,baseline[1],300,"release restores animation-free controller hand");
    g_motionGun=false;Check(BuildArmPalette(context,1)==nullptr,"motion-arm toggle restores game path");
    g_armIk84=true; // all subsequent physical reload regressions use production policy
    g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;
    context->UpdateSubresource(cb[3],0,nullptr,original,0,0);
    context->VSSetConstantBuffers(0,4,cb);
    g_actionProbe=ActionProbeState{};g_actionProbe.begin=g_frame;g_actionProbe.event=1;
    g_actionProbe.resourceCount=64; // bypass disk mesh exports in this GPU-palette fixture
    g_actionProbe.file=CreateFileW(L"action_probe_fixture.bin",GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    Check(g_actionProbe.file!=INVALID_HANDLE_VALUE,"probe fixture file");
    DrawCall action={};action.a=6795;action.b=9633;action.d=7;
    RecordActionDraw(context,action);
    Check(g_actionProbe.draws==1,"one raw action record");
    SetFilePointer(g_actionProbe.file,0,nullptr,FILE_BEGIN);
    ActionProbeRecord captured={};DWORD probeRead=0;
    Check(ReadFile(g_actionProbe.file,&captured,sizeof(captured),&probeRead,nullptr)&&probeRead==sizeof(captured),"trace readback");
    Check(captured.magic==0x41505231&&captured.version==1&&captured.event==1,"trace version and event");
    Check(captured.a==6795&&captured.b==9633&&captured.base==7&&captured.stride==40,"draw identity");
    Check(captured.cbBytes[3]==2304&&captured.cbFloats[3]==576,"complete bone palette");
    Compare(captured.cb[3],original,576,"probe stores original GPU bones");
    float untouched[576];Read(device,context,cb[3],untouched);Compare(untouched,original,576,"probe does not mutate game buffer");
    CloseActionProbe("fixture finished");
    std::puts("PASS: action probe raw GPU palettes, draw identity, file format, no buffer mutation");
    g_oCopySubresourceRegion=EjectCopyRange;
    D3D11_BUFFER_DESC effectDesc={};effectDesc.ByteWidth=256;effectDesc.Usage=D3D11_USAGE_DYNAMIC;
    effectDesc.BindFlags=D3D11_BIND_VERTEX_BUFFER;effectDesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* effect=nullptr;Check(SUCCEEDED(device->CreateBuffer(&effectDesc,nullptr,&effect)),"dynamic effect buffer");
    unsigned char effects[256];for(int i=0;i<256;++i)effects[i]=static_cast<unsigned char>(i);
    D3D11_MAPPED_SUBRESOURCE effectMap={};Check(SUCCEEDED(context->Map(effect,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"effect map");
    memcpy(effectMap.pData,effects,256);context->Unmap(effect,0);
    ID3D11Buffer* effectIndices=Buffer(device,12,D3D11_BIND_INDEX_BUFFER);
    const unsigned short effectIndexData[6]={99,99,2,4,3,99};context->UpdateSubresource(effectIndices,0,nullptr,effectIndexData,0,0);
    UINT effectStride=16,effectOffset=16;context->IASetVertexBuffers(0,1,&effect,&effectStride,&effectOffset);
    context->IASetIndexBuffer(effectIndices,DXGI_FORMAT_R16_UINT,2);
    effectDesc.ByteWidth=64;ID3D11Buffer* effectInstance=nullptr;
    Check(SUCCEEDED(device->CreateBuffer(&effectDesc,nullptr,&effectInstance)),"effect instance stream");
    Check(SUCCEEDED(context->Map(effectInstance,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"instance map");
    for(int i=0;i<64;++i)static_cast<unsigned char*>(effectMap.pData)[i]=static_cast<unsigned char>(200+i);
    context->Unmap(effectInstance,0);UINT instanceStride=16,instanceOffset=8;
    context->IASetVertexBuffers(1,1,&effectInstance,&instanceStride,&instanceOffset);
    g_frame=1000;g_ejection=EjectionState{};g_ejection.sequence=1;g_ejection.begun=GetTickCount64();g_ejection.burstUntil=g_ejection.begun+2000;
    g_ejection.file=CreateFileW(L"ejection_probe_fixture.bin",GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    Check(g_ejection.file!=INVALID_HANDLE_VALUE,"ejection fixture file");
    g_chambered=false;g_pumpStage=1;g_twoHand=true;g_slideGrab=.1;g_slide=.04;MarkEjectionProbe(1);
    DrawCall effectCall={2,3,2,1,2,1};RecordEjectionDraw(context,effectCall,true);
    Check(g_ejection.draws==1,"first effect snapshot");
    for(int i=0;i<256;++i)effects[i]=static_cast<unsigned char>(255-i);
    Check(SUCCEEDED(context->Map(effect,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"effect reused map");
    memcpy(effectMap.pData,effects,256);context->Unmap(effect,0);g_frame=1002;
    Check(SUCCEEDED(context->Map(effectInstance,0,D3D11_MAP_WRITE_DISCARD,0,&effectMap)),"reused instance map");
    for(int i=0;i<64;++i)static_cast<unsigned char*>(effectMap.pData)[i]=static_cast<unsigned char>(100+i);
    context->Unmap(effectInstance,0);
    RecordEjectionDraw(context,effectCall,true);Check(g_ejection.draws==2,"reused effect snapshot");
    g_chambered=true;g_slide=.1;MarkEjectionProbe(2);
    SetFilePointer(g_ejection.file,0,nullptr,FILE_BEGIN);
    auto fileRead=[&](void* ptr,UINT size){DWORD n=0;Check(ReadFile(g_ejection.file,ptr,size,&n,nullptr)&&n==size,"ejection chunk read");};
    EjectionHeader ejectHeader={};EjectionMarker ejectMarker={};fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&ejectMarker,sizeof(ejectMarker));
    Check(ejectHeader.magic==0x45505231&&ejectHeader.type==0&&ejectMarker.event==1&&!ejectMarker.chambered,"rear marker recorded separately");
    for(int snapshot=0;snapshot<2;++snapshot)
    {
        EjectionDraw capturedEffect={};fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&capturedEffect,sizeof(capturedEffect));
        Check(ejectHeader.type==1&&ejectHeader.bytes==sizeof(capturedEffect)+48+6+64,"variable payload size");
        Check(capturedEffect.vertexSliceOffset==64&&capturedEffect.vertexSliceBytes==48,"VB range includes stream offset and base vertex");
        Check(capturedEffect.indexSliceOffset==4&&capturedEffect.indexSliceBytes==6,"IB range includes start index and binding offset");
        Check(capturedEffect.raw.vertexBuffer==reinterpret_cast<uintptr_t>(effect),"same reused buffer identity");
        unsigned char verts[48],ids[6];fileRead(verts,48);fileRead(ids,6);
        for(int i=0;i<48;++i)Check(verts[i]==static_cast<unsigned char>(snapshot?255-(64+i):64+i),"captures changing contents of reused effect buffer");
        Check(memcmp(ids,effectIndexData+2,6)==0,"index snapshot exact");
        Check(capturedEffect.raw.kind==2&&capturedEffect.raw.b==2&&capturedEffect.raw.c==1&&capturedEffect.raw.e==2,"instanced draw arguments preserved");
        Check(capturedEffect.extra[0].buffer==reinterpret_cast<uintptr_t>(effectInstance)&&capturedEffect.extra[0].offset==8&&capturedEffect.extra[0].snapshotBytes==64,"instance stream metadata");
        unsigned char instanceBytes[64];fileRead(instanceBytes,64);
        for(int i=0;i<64;++i)Check(instanceBytes[i]==static_cast<unsigned char>((snapshot?100:200)+i),"changing instance stream captured");
        Check(capturedEffect.cbShadowMask&(1u<<3),"original bone shadow copied");Compare(capturedEffect.raw.cb[3],original,576,"effect raw palette unchanged");
    }
    fileRead(&ejectHeader,sizeof(ejectHeader));fileRead(&ejectMarker,sizeof(ejectMarker));Check(ejectHeader.type==0&&ejectMarker.event==2&&ejectMarker.chambered,"forward marker recorded separately");
    std::vector<unsigned char> rejected;Check(!EjectionReadRange(context,effect,250,16,rejected),"out of bounds effect slice rejected");
    g_ejection.draws=4096;RecordEjectionDraw(context,effectCall,true);
    Check(g_ejection.dataLimited&&g_ejection.file!=INVALID_HANDLE_VALUE,"geometry cap preserves pump-marker channel");
    g_lastShotFrame=-100000;g_ejection.pulled=g_ejection.returned=false;
    g_chambered=false;g_pumpStage=1;g_twoHand=false;TickEjectionProbe(true);
    Check(!g_ejection.pulled,"one-handed pose cannot create physical pull marker");
    g_twoHand=true;TickEjectionProbe(true);Check(g_ejection.pulled&&!g_ejection.returned,"tick recognizes rearward threshold");
    g_chambered=true;TickEjectionProbe(true);Check(g_ejection.returned,"tick recognizes forward completion");
    EndEjectionProbe("fixture finished");effect->Release();effectIndices->Release();effectInstance->Release();
    std::puts("PASS: ejection probe live buffer reuse, bounded vertex/index slices, raw palette and rear/forward markers");
    if(GetEnvironmentVariableW(L"FC3_MACHETE118_ONLY",nullptr,0)){MH_Initialize();TestMacheteHolding(device,context);TestMacheteMelee(device);TestMacheteHoldModes(device);TestMacheteBladeContact();TestMacheteCollider118();TestMacheteActors119();TestMachetePawn120();TestMacheteFeedback121();TestMacheteRecovery123();TestMacheteColliderGuide118(device,context);return 0;}
    TestVehicleWheel187();
    TestVehicleLook();
    TestGlider257();
    TestNativeFoot281();TestNativeAction282();
    TestTriggerFinger262();
    TestPhysicalHull(device,context);
    TestReloadShellVisual(device,context);
    TestAmmoProbe();
    TestAmmoOwner();
    TestAutoAmmoCapture();
    TestPhysicalReload();
    TestShotgunControlOwnership();
    TestAnimationTimeline(device,context);
    TestCapturedHands(device,context);
    TestShotHandSprint107();
    TestShotAim107();
    TestMuzzleFlash111();
    TestReloadPoseLifecycle(device,context);
    TestReadOnlyReloadCountTrace();
    TestOneShellX(device,context);
    TestMacheteHolding(device,context);
    TestMacheteMelee(device);
    TestMacheteHoldModes(device);
    TestMacheteBladeContact();
    TestMacheteCollider118();TestMacheteActors119();TestMachetePawn120();TestMacheteFeedback121();TestMacheteRecovery123();TestMacheteWorld126();TestFastTurnCapture126();TestCameraOwnership127();
    TestMacheteColliderGuide118(device,context);
    TestMacheteNativeProbe();
    TestMacheteRoutingProbe();
    TestMacheteQuickProbe();
    TestPumpFireLock();
    TestSweptBlade();
    TestMacheteOwnership(device);
    TestViewOwnership(device,context);
    TestPistolReload(device,context);
    TestPistolMechanics(device,context);TestSkorpion(device,context);
    TestPistolGuides(device,context);TestPistolPreview(device,context);TestPistolBoxes();TestPersistentAmmoPreview(device,context);TestAk(device,context);TestP6p9(device,context);TestRifles(device,context);TestLever1887(device,context);TestLever1887Shells();TestLever1887Simple();TestShopUnlock();TestRevolver();TestFlare();TestRpg();TestGm94();TestFlamer();TestSpas();TestD50(device,context);TestWeapons7Guides();TestWeapons8Capture(device,context);g_oCopyResource=ProbeCopy;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;TestRepairIndexReadback(device,context);TestPistolEarlyPiece();TestPistolAttachmentAndSupport();TestRepairToolEffects();TestBow(device,context);TestBowReload();
    TestSpecial248(device,context);TestSpecial247(device,context);TestRevolverAssembly217(device,context);TestHealProbe217(device,context);
    TestWeaponPaletteBindings218(device,context);
    TestNativeSingleShots219();
    TestMechanismInterruptions219();
    TestShellRecovery221();TestBoltRegrab221();
    TestFireOwnership222();
    TestClipPause223();
    TestMagazineRecovery224();
    sound225test::TestD50Sound225();
    seat226test::TestD50Seat226();
    smg229test::Test(device,context);
    skseat230::Test();
    seat313test::Test();seat315test::Test();
    TestSpecial239(device,context);
    TestSupportContact236();
    TestSvdLens236(device,context);
    TestRifleCasings235(device,context);
    assault234::Test(device,context);
    TestShotgun231(device,context);
    TestActionCost228(device,context);
    TestSlideTint49(device,context);
    TestWeaponAttachments220(device,context);
    context->ClearState();context->Flush();for(auto* b:cb)if(b)b->Release();vb->Release();g_armPaletteCb[0]->Release();g_armPaletteCb[1]->Release();g_bodyPaletteCb->Release();context->Release();device->Release();
    std::puts("PASS: free-arm animation suppressed; sleeves/body turns agree; armed right and held-left grip paths preserved; release restores stable pose; toggle restores game path.");
}
