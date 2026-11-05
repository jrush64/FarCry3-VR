// This boundary is AFTER physics-id -> entity resolution, self rejection and
// the engine's rigid/static damage-route classification. Collider contacts go straight to
// the unchanged native melee event constructor/dispatcher/destructor; the
// camera LOS and camera hit-volume ray are not used for a physical trigger.
constexpr UINT kMacheteActorSite118=0x12d7cf3;
constexpr BYTE kMacheteActorSignature118[16]={0x8b,0x7c,0x24,0x18,0xf3,0x0f,0x10,0x47,0x14,0xf3,0x0f,0x10,0x4f,0x18,0xf3,0x0f};
void* g_macheteActorOriginal118=nullptr;
UINT g_macheteActorEvent118=0,g_macheteActorSkip118=0;
UINT __cdecl MacheteActorContact118(UINT* saved)
{
    if(!saved||!MacheteBladeInCall())return 0;
    auto& b=g_macheteBladeContact;if(!b.queryApplied118)return 2;
    const UINT stack=saved[3]+4;UINT contact=0,config=0;
    if(!ReloadTraceRead(stack+0x18,&contact,4)||!contact||!ReloadTraceRead(saved[2]+12,&config,4)||config!=b.callConfig118)return 2;
    const UINT entity=MacheteNativeRead(saved[4],12);if(!entity)return 2;
    const ULONGLONG now=GetTickCount64();if(MacheteBladeOnCooldown(entity,now))return 2;
    float point[3]={};if(!ReloadTraceRead(contact+0x14,point,12))return 2;
    const V3 p={point[0],point[1],point[2]},motion=Sub(b.end,b.start);const double travel2=Dot(motion,motion);
    const double t=travel2>1e-9?(std::max)(0.0,(std::min)(1.0,Dot(Sub(p,b.start),motion)/travel2)):0;
    if(!std::isfinite(Length(p))||Length(Sub(p,Add(b.start,Scale(motion,t))))>b.half+b.radius+.05)return 2;
    // The native post-event path destroys the camera ray's temporary list.
    // That ray was skipped, so its data/count/capacity must be initialized as
    // an empty list before entering the shared event/cleanup block.
    const UINT empty[3]={};V3 impulse=Unit(motion);if(Length(impulse)<.5)impulse=b.axis;
    const float direction[3]={float(impulse.x),float(impulse.y),float(impulse.z)};
    __try{memcpy(reinterpret_cast<void*>(stack+0x3c),empty,sizeof(empty));memcpy(reinterpret_cast<void*>(stack+0x94),direction,sizeof(direction));}
    __except(EXCEPTION_EXECUTE_HANDLER){return 2;}
    MacheteBladeSetCooldown(entity,now);
    saved[0]=contact+0x14; // EDI: real physics hit position
    saved[1]=config;      // ESI: private per-call damage configuration
    ++b.actorContacts118;return 1;
}
__declspec(naked) void MacheteActorBoundary118()
{
    __asm pushfd
    __asm pushad
    __asm cld
    __asm mov eax,esp
    __asm sub esp,544
    __asm and esp,0fffffff0h
    __asm mov [esp+512],eax
    __asm fxsave [esp]
    __asm push eax
    __asm call MacheteActorContact118
    __asm add esp,4
    __asm mov [esp+516],eax
    __asm fxrstor [esp]
    __asm cmp dword ptr [esp+516],1
    __asm je contact118
    __asm cmp dword ptr [esp+516],2
    __asm je skip118
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_macheteActorOriginal118]
contact118:
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_macheteActorEvent118]
skip118:
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_macheteActorSkip118]
}
