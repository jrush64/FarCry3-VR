#define FC3_THROW_PROBE_TEST 1
#include "../src/throwable_launch_probe.cpp"
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace fc3throw;
void Check(bool ok,const char* label) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);} }
static void* g_entry=nullptr;
static UINT g_strategy=0,g_projectile=0,g_event=0;
static float g_seed[4]={1.25f,-2.5f,3.75f,-4.f},g_sse[4]={},g_x87=0;
static const float kX87=9.25f;
static UINT g_registers[6]={},g_flags=0;
__declspec(naked) void EventSink() { __asm ret 4 }
__declspec(naked) void CallFixture(UINT,UINT) {
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        mov ebx,[g_strategy]
        mov edi,ebx
        mov esi,[g_event]
        mov ecx,[g_projectile]
        lea eax,EventSink
        cmp dword ptr [ebp+8],5
        jne fixture_ready
        mov edi,esi
    fixture_ready:
        fninit
        fld dword ptr [kX87]
        movups xmm0,[g_seed]
        stc
        std
        call dword ptr [g_entry]
        mov [g_registers],eax
        mov [g_registers+4],ecx
        mov [g_registers+8],edx
        mov [g_registers+12],ebx
        mov [g_registers+16],esi
        mov [g_registers+20],edi
        movups [g_sse],xmm0
        fstp dword ptr [g_x87]
        pushfd
        pop eax
        mov [g_flags],eax
        cld
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
#include "manual_launch143_fixture.inl"
#include "throwable_update146_fixture.inl"
#include "throwable_stock147_fixture.inl"
#include "grenade_safety155_fixture.inl"
#include "grenade_release156_fixture.inl"
#include "d50_audio170_fixture.inl"
#include "placeables177_native_fixture.inl"
int main() {
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(page!=nullptr,"fixture memory");const UINT a=reinterpret_cast<UINT>(page);
    const UINT strategy=a,wrapper=a+1024,ownerRef=a+2048,owner=a+3072,
               projectileRef=a+4096,projectile=a+5120,inventory=a+6144,event=a+7168,table=a+8192;
    g_engine=0x10000000;g_enabled=1;g_strategy=strategy;g_projectile=projectile;g_event=event;
    *reinterpret_cast<UINT*>(strategy)=g_engine+kStrategyVtable;
    *reinterpret_cast<UINT*>(strategy+0x38)=wrapper;
    *reinterpret_cast<UINT*>(strategy+0xf4)=projectileRef;
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x000006e79d52dd0cULL;
    // Distinct weapon and owning pawn: reproduces the failed live run rather
    // than making the weapon ID equal Jason's ID.
    const UINT weaponRef=a+11264,weaponEntity=a+12288;
    *reinterpret_cast<UINT*>(wrapper+4)=weaponRef;
    *reinterpret_cast<UINT*>(weaponRef+12)=weaponEntity;
    *reinterpret_cast<ULONGLONG*>(weaponEntity+8)=0xb566ca78807a5500ULL;
    *reinterpret_cast<UINT*>(wrapper+0x28)=ownerRef;
    *reinterpret_cast<ULONGLONG*>(ownerRef)=0x1111222233334444ULL;
    *reinterpret_cast<UINT*>(wrapper+0x3c)=inventory;
    const UINT execution=a+13312,executionRef=a+13824,executionEntity=a+14336,config=a+10240;
    *reinterpret_cast<UINT*>(execution+4)=executionRef;*reinterpret_cast<UINT*>(executionRef+12)=executionEntity;*reinterpret_cast<UINT*>(execution+0x24)=config;
    *reinterpret_cast<ULONGLONG*>(strategy+8)=*reinterpret_cast<ULONGLONG*>(executionEntity+8)=0x1122334400009900ULL;

    *reinterpret_cast<UINT*>(ownerRef+0xc)=owner;
    *reinterpret_cast<ULONGLONG*>(owner+8)=0x1111222233334444ULL;
    *reinterpret_cast<UINT*>(projectileRef+0xc)=projectile;
    *reinterpret_cast<ULONGLONG*>(projectile+8)=0x5555666677778888ULL;
    *reinterpret_cast<UINT*>(projectile)=table;
    *reinterpret_cast<UINT*>(table+0x28)=reinterpret_cast<UINT>(EventSink);
    *reinterpret_cast<UINT*>(inventory+0x60)=4;
    for(int i=0;i<3;++i) {
        reinterpret_cast<float*>(strategy+0x88)[i]=100.f+i;
        reinterpret_cast<float*>(strategy+0xc8)[i]=5.f+i;
        reinterpret_cast<float*>(strategy+0xd4)[i]=-2.f+i;
        reinterpret_cast<float*>(projectile+0x50)[i]=200.f+i;
    }
    *reinterpret_cast<UINT*>(event)=0x11fcd63c;
    PoseSample pose;pose.wallMs=GetTickCount64();pose.localOwnerId=0x1111222233334444ULL;
    pose.frame=17;pose.epoch=3;pose.validity=0xf;pose.position[0]=10;pose.orientation[3]=1;
    PublishPose(pose);
    UINT saved[9]={};saved[0]=strategy;saved[1]=event;saved[4]=strategy;saved[6]=projectile;
    std::vector<BYTE> before(page,page+16384);
    for(UINT phase=1;phase<=6;++phase) {
        saved[0]=phase==5?event:strategy;saved[1]=phase==1?execution:event;Observe(phase,saved);
    }
    Check(g_queued==6&&g_queue[0].archetype==0x000006e79d52dd0cULL&&g_queue[0].counter==4&&
          g_queue[1].origin[0]==100.f&&g_queue[3].velocity[2]==7.f&&g_queue[2].eventWords[0]==0x11fcd63c&&
          g_queue[4].dispatchTarget==projectile&&g_queue[5].pose.epoch==3&&g_queue[0].flags==0x3bf,
          "six observations retain IDs, vectors, counter, event bytes, local-owner filter and supplied pose");
    Check(!std::memcmp(before.data(),page,16384),"observations do not modify engine-style object, inventory or event memory");
    HANDLE f=CreateFileW(L"prototype-capture.tlp1",GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(f!=INVALID_HANDLE_VALUE&&FlushTo(f),"durable write");CloseHandle(f);
    Check(g_written==6*sizeof(LaunchRecord)&&g_queued==0,"complete record bytes flushed");
    *reinterpret_cast<UINT*>(strategy)=0;Observe(2,saved);Check(g_queued==0,"unrelated vtable ignored");
    *reinterpret_cast<UINT*>(strategy)=g_engine+kStrategyVtable;
    pose.localOwnerId=1;PublishPose(pose);Observe(2,saved);Check(g_queued==0,"other owner's launch ignored");
    pose.localOwnerId=0;PublishPose(pose);
    saved[4]=1;Observe(2,saved);Check(g_queued==0,"unreadable object safely ignored");saved[4]=strategy;
    for(int i=0;i<70;++i)Observe(2,saved);
    Check(g_queued==64&&g_lost==6,"bounded queue reports lost observations");g_queued=0;
    // Exercise the exact displaced instructions with real MinHook relocation.
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(code!=nullptr&&MH_Initialize()==MH_OK,"fixture detour library");
    constexpr UINT prefix[6]={6,6,5,6,5,5};
    for(UINT i=0;i<6;++i) {
        std::memset(code,0xcc,4096);BYTE* site=code;
        if(i==5){code[0]=0x57;site=code+1;} // original site's pop EDI has a matching push
        std::memcpy(site,kSignatures[i],prefix[i]);
        UINT tail=prefix[i];
        if(i==3){site[tail++]=0xdd;site[tail++]=0xd8;} // discard original FLD result; retain x87 seed
        site[tail]=0xc3;FlushInstructionCache(GetCurrentProcess(),code,64);
        g_entry=code;CallFixture(i+1,wrapper);
        UINT baseline[6];std::memcpy(baseline,g_registers,sizeof(baseline));const UINT flags=g_flags;
        Check(!std::memcmp(g_seed,g_sse,sizeof(g_seed))&&g_x87==kX87,"unhooked original instruction baseline");
        Check(MH_CreateHook(site,g_bridges[i],&g_original[i])==MH_OK&&MH_EnableHook(site)==MH_OK,"exact launch-site detour enabled");
        g_queued=0;volatile UINT canary=0xabcdef12;CallFixture(i+1,wrapper);
        Check(canary==0xabcdef12&&g_queued==1&&g_queue[0].phase==i+1,"detoured native instructions maintain stack and record phase");
        Check(!std::memcmp(baseline,g_registers,sizeof(baseline))&&((g_flags^flags)&0x401)==0&&
              !std::memcmp(g_seed,g_sse,sizeof(g_seed))&&g_x87==kX87,
              "all six bridges restore integer/EFLAGS/SSE/x87 after deliberate observer clobbers");
        Check(MH_DisableHook(site)==MH_OK&&MH_RemoveHook(site)==MH_OK,"detour cleanup");g_original[i]=nullptr;
    }
    Check(MH_Uninitialize()==MH_OK,"fixture library cleanup");
    TestManualLaunch143(a,strategy,wrapper,owner,projectileRef,projectile,inventory,event,execution);
    TestUpdateBoundary146(execution);
    TestStock147(strategy,wrapper,inventory);
    TestGrenadeSafety155(strategy,execution,projectile,inventory,event);
    TestRelease156(strategy,execution,projectile,inventory,event);
    TestD50Audio170();
    TestPlaceables177();
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,kImageSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(image!=nullptr,"signature image");
    for(UINT i=0;i<6;++i)std::memcpy(image+kSites[i],kSignatures[i],16);
    Check(SignaturesMatch(image,kImageSize)&&!SignaturesMatch(image,0),"complete image and bounds guard");
    for(UINT i=0;i<6;++i){image[kSites[i]]^=1;Check(!SignaturesMatch(image,kImageSize),"each changed site refuses installation");image[kSites[i]]^=1;}
    memcpy(image+0x128dae0,kUpdateBytes146,16);memcpy(image+0x128da40,kReleaseBytes146,16);
    const UINT imageBase=reinterpret_cast<UINT>(image);*reinterpret_cast<UINT*>(image+kStrategyVtable+0x4c)=imageBase+0x128dae0;*reinterpret_cast<UINT*>(image+kStrategyVtable+0x2c)=imageBase+0x128da40;
    Check(ReleaseSignatures146(image),"native update/release entries and exact strategy virtual slots verified");
    image[0x128da40]^=1;Check(!ReleaseSignatures146(image),"changed release signature refuses early dispatch");image[0x128da40]^=1;
    *reinterpret_cast<UINT*>(image+kStrategyVtable+0x2c)=0;Check(!ReleaseSignatures146(image),"changed native release virtual slot refuses early dispatch");
    Check(!Install(),"fixture cannot install into an absent game engine");
    g_queued=1;g_writeError=0;
    Check(!FlushTo(INVALID_HANDLE_VALUE)&&Status().writeError!=0,"write failure visible without claiming capture success");
    g_queued=1;g_written=8*1024*1024;g_full=false;
    f=CreateFileW(L"prototype-cap-test.tlp1",GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(f!=INVALID_HANDLE_VALUE&&FlushTo(f)&&Status().full,"file cap bounds observations");CloseHandle(f);
    VirtualFree(image,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: six exact MinHook sites; stack/integer/EFLAGS/SSE/x87 preservation; read-only IDs/vectors/counter/events; owner filtering; bounded FIFO; durable records, write-error and signature guards. No game launched.");
    return 0;
}
