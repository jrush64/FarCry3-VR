// Candidate118 live failure: scenery convex contacts do not supply guards.
// Use the native character broad phase, then ray-test the returned physics
// bodies against the swept blade. Never accept the native camera cone.
struct MacheteActorState119 {
    bool pass=false,query=false;UINT self=0,count=0;
    struct Hit {UINT wrapper;V3 point;} hits[64];
    UINT searches=0,bodies=0,rays=0,intersections=0,selected=0,dispatches=0,errors=0;
    ULONGLONG contactMs=0,dispatchMs=0;
} g_macheteActor119;
void* g_actorSearchOriginal119=nullptr;void* g_actorBodyOriginal119=nullptr;
void* g_actorSelectOriginal119=nullptr;void* g_actorDispatchOriginal119=nullptr;
UINT g_actorInclude119=0,g_actorReject119=0,g_actorFinish119=0;
UINT g_actorObservationFinish121=0;
void* g_pawnRouteOriginal121=nullptr;UINT g_pawnRouteCleanup121=0;
#include "machete_world126.inl"
UINT __cdecl MachetePawnRoute121(UINT* saved)
{
    if(!saved||!MacheteBladeInCall())return 0;
    auto& b=g_macheteBladeContact;const UINT wrapper=saved[4],entity=MacheteNativeRead(wrapper,12);
    if(!entity||!MachetePawnComponent120(entity))return 0; // retain native non-pawn behavior
    if(!b.queryApplied118||(g_macheteCuts121.active&&!g_macheteCuts121.damage))return 1;
    const UINT contact=MacheteNativeRead(saved[3]+4,0x18);float xyz[3]={};
    if(!contact||!ReloadTraceRead(contact+0x14,xyz,12))return 1;
    const V3 point={xyz[0],xyz[1],xyz[2]},motion=Sub(b.end,b.start);const double travel=Dot(motion,motion);
    const double t=travel>1e-9?(std::max)(0.,(std::min)(1.,Dot(Sub(point,b.start),motion)/travel)):0.;
    if(!std::isfinite(Length(point))||Length(Sub(point,Add(b.start,Scale(motion,t))))>b.half+b.radius+.05)return 1;
    MacheteCutTouch121(entity);MachetePawnContact120(wrapper,point);
    return 1; // native wrapper cleanup, never a second unguarded pawn dispatch
}

bool MacheteBodyRay119(UINT body,V3 from,V3 to,V3& point)
{
    if(g_macheteBladeContact.exceptions)return false;
    const V3 delta=Sub(to,from);const double length=Length(delta);if(length<1e-6)return false;
    const UINT fn=MacheteNativeRead(MacheteNativeRead(body,0),0xa8);if(!fn)return false;
    const float origin[3]={float(from.x),float(from.y),float(from.z)},direction[3]={float(delta.x),float(delta.y),float(delta.z)};
    __declspec(align(16)) BYTE result[64]={};memset(result+0x24,0xff,8);
    const float minusOne=-1.f;memcpy(result+0x1c,&minusOne,4);
    ++g_macheteActor119.rays;
    bool hit=false;
    __try {hit=reinterpret_cast<bool(__thiscall*)(void*,const float*,const float*,void*,int)>(fn)(reinterpret_cast<void*>(body),origin,direction,result,1);}
    __except(EXCEPTION_EXECUTE_HANDLER){++g_macheteActor119.errors;++g_macheteBladeContact.exceptions;return false;}
    if(!hit)return false;
    float xyz[3];memcpy(xyz,result,12);point={xyz[0],xyz[1],xyz[2]};if(!std::isfinite(Length(point)))return false;
    const double t=(std::max)(0.,(std::min)(1.,Dot(Sub(point,from),delta)/Dot(delta,delta)));
    return Length(Sub(point,Add(from,Scale(delta,t))))<.015;
}
bool MacheteBodyContact119(UINT body,V3& point)
{
    const auto& b=g_macheteBladeContact;
    const V3 axis=Scale(b.axis,b.half+b.radius),motion=Sub(b.end,b.start);
    V3 right=Unit(Cross(b.axis,motion));if(Length(right)<.5)right=Unit(Cross(b.axis,fabs(b.axis.z)<.9?V3{0,0,1}:V3{0,1,0}));
    const V3 up=Unit(Cross(b.axis,right));
    // Longitudinal sections and crossing motion diagonals, with samples on
    // the circumference. This samples the configured capsule; no body-size
    // estimate, view direction or distance-only damage test is involved.
    for(int ring=0;ring<5;++ring){
        const V3 offset=ring==0?V3{}:Scale(ring<3?right:up,(ring%2?1.:-1.)*b.radius);
        const V3 p=Add(b.start,offset),q=Add(b.end,offset);
        const V3 a=Sub(p,axis),c=Add(p,axis),d=Sub(q,axis),e=Add(q,axis);
        if(MacheteBodyRay119(body,a,c,point)||MacheteBodyRay119(body,d,e,point)||MacheteBodyRay119(body,a,e,point)||MacheteBodyRay119(body,c,d,point))return true;
    }
    return false;
}
void __cdecl MacheteActorSearch119(UINT* saved)
{
    auto& a=g_macheteActor119;if(!MacheteBladeInCall()||!a.pass)return;
    auto& b=g_macheteBladeContact;const UINT stack=saved[3]+4;
    a.query=b.queryApplied118;a.count=0;a.self=MacheteNativeRead(stack,0x18);++a.searches;
    if(!a.query)return;
    if(g_macheteCuts121.active)++g_macheteCuts121.queries;
    const V3 middle=Scale(Add(b.start,b.end),.5);const float origin[3]={float(middle.x),float(middle.y),float(middle.z)},direction[3]={0,1,0};
    const float radius=float(b.half+b.radius+Length(Sub(b.end,b.start))*.5+.02),angle=180.f;
    __try{memcpy(reinterpret_cast<void*>(stack+0x5c),origin,12);memcpy(reinterpret_cast<void*>(stack+0x3c),direction,12);memcpy(reinterpret_cast<void*>(stack+0x10),&radius,4);memcpy(reinterpret_cast<void*>(stack+0x38),&angle,4);}
    __except(EXCEPTION_EXECUTE_HANDLER){a.query=false;++a.errors;}
}
UINT __cdecl MacheteActorBody119(UINT* saved)
{
    auto& a=g_macheteActor119;if(!MacheteBladeInCall()||!a.pass)return 0;if(!a.query)return 2;
    const UINT wrapper=saved[0],entity=MacheteNativeRead(wrapper,12);if(!entity||entity==MacheteNativeRead(a.self,12))return 2;
    if(MacheteNativeRead(entity,0x60)&16)return 2;
    const UINT stack=saved[3]+4,data=MacheteNativeRead(stack,0x48),index=saved[4],count=MacheteNativeRead(stack,0x4c);
    if(!data||index>=count||count>4096)return 2;
    const UINT body=MacheteNativeRead(data,index*4);if(!body)return 2;++a.bodies;
    if(MacheteCutBroadHold121(entity)){a.contactMs=GetTickCount64();return 2;}
    if(g_macheteCuts121.active&&!g_macheteCuts121.damage)return 2;
    V3 point;if(!MacheteBodyContact119(body,point))return 2;
    MacheteCutTouch121(entity);
    if(a.count>=64)return 2;
    a.hits[a.count++]={wrapper,point};++a.intersections;a.contactMs=GetTickCount64();
    return 1; // native append retains wrapper; native vector cleanup releases it
}
void ReleaseMacheteActorRef119(UINT wrapper)
{
    if(!wrapper)return;
    if(InterlockedDecrement(reinterpret_cast<volatile LONG*>(wrapper+8))>0)return;
    const UINT fn=g_macheteBladeContact.engine+0x13b5c0;
    __asm push esi
    __asm mov esi,wrapper
    __asm call fn
    __asm pop esi
}
UINT __cdecl MacheteActorSelect119(UINT* saved)
{
    auto& a=g_macheteActor119;if(!MacheteBladeInCall()||!a.pass)return 0;
    const UINT stack=saved[3]+4,frame=saved[2];auto& b=g_macheteBladeContact;
    const UINT data=MacheteNativeRead(stack,0x240),count=MacheteNativeRead(stack,0x244);
    UINT chosen=0;V3 point={};
    if(a.query&&(!g_macheteCuts121.active||g_macheteCuts121.damage)&&count<=4096)for(UINT i=0;i<count&&!chosen;++i){
        const UINT wrapper=MacheteNativeRead(data,i*4),entity=MacheteNativeRead(wrapper,12);
        if(!entity||entity==MacheteNativeRead(a.self,12)||MacheteBladeOnCooldown(entity,GetTickCount64()))continue;
        for(UINT j=0;j<a.count;++j)if(a.hits[j].wrapper==wrapper){
            // Native rigid/static event classification consumes its wrapper
            // reference. False is a different damage route, not immunity.
            InterlockedIncrement(reinterpret_cast<volatile LONG*>(wrapper+8));
            if(reinterpret_cast<bool(__cdecl*)(UINT)>(b.engine+0x12d2250)(wrapper)){chosen=wrapper;point=a.hits[j].point;}
            else MachetePawnContact120(wrapper,a.hits[j].point);
            break;
        }
    }
    const BYTE block=chosen?0:1;memcpy(reinterpret_cast<void*>(frame+0x14),&block,1);
    if(chosen){
        const UINT previous=MacheteNativeRead(stack,0x14);
        InterlockedIncrement(reinterpret_cast<volatile LONG*>(chosen+8));
        memcpy(reinterpret_cast<void*>(stack+0x14),&chosen,4);ReleaseMacheteActorRef119(previous);
        const V3 motion=Unit(Sub(b.end,b.start));const V3 impulse=Length(motion)>.5?motion:b.axis;
        const float hit[3]={float(point.x),float(point.y),float(point.z)},direction[3]={float(impulse.x),float(impulse.y),float(impulse.z)};
        memcpy(reinterpret_cast<void*>(stack+0x68),hit,12);memcpy(reinterpret_cast<void*>(stack+0x3c),direction,12);
        MacheteBladeSetCooldown(MacheteNativeRead(chosen,12),GetTickCount64());++a.selected;
    }
    a.query=false;
    if(g_macheteCuts121.active&&!g_macheteCuts121.damage){
        // Observation must not emit the native no-target/miss feedback at
        // 12D6AD1. Shared cleanup12D6A42 expects ESI=the owned selection.
        saved[1]=MacheteNativeRead(stack,0x14);return 2;
    }
    return 1; // jump past camera-distance candidate selection
}
void __cdecl MacheteActorDispatch119(UINT*)
{
    if(!MacheteBladeInCall()||!g_macheteActor119.pass)return;
    auto& a=g_macheteActor119;++a.dispatches;a.dispatchMs=GetTickCount64();
    auto& b=g_macheteBladeContact;++b.events;++b.actorHits;b.lastEvent=a.dispatchMs;
}
#define ACTOR_OBSERVE119(name,fn,original) \
__declspec(naked) void name(){ \
    __asm pushfd \
    __asm pushad \
    __asm cld \
    __asm mov eax,esp \
    __asm sub esp,544 \
    __asm and esp,0fffffff0h \
    __asm mov [esp+512],eax \
    __asm fxsave [esp] \
    __asm push eax \
    __asm call fn \
    __asm add esp,4 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [original] \
}
ACTOR_OBSERVE119(MacheteWorldBoundary126,MacheteWorldContact126,g_macheteWorldOriginal126)
ACTOR_OBSERVE119(MacheteActorSearchBoundary119,MacheteActorSearch119,g_actorSearchOriginal119)
ACTOR_OBSERVE119(MacheteActorDispatchBoundary119,MacheteActorDispatch119,g_actorDispatchOriginal119)
#undef ACTOR_OBSERVE119
#define ACTOR_ROUTE119(name,fn,original,yes,no) \
__declspec(naked) void name(){ \
    __asm pushfd \
    __asm pushad \
    __asm cld \
    __asm mov eax,esp \
    __asm sub esp,544 \
    __asm and esp,0fffffff0h \
    __asm mov [esp+512],eax \
    __asm fxsave [esp] \
    __asm push eax \
    __asm call fn \
    __asm add esp,4 \
    __asm mov [esp+516],eax \
    __asm fxrstor [esp] \
    __asm cmp dword ptr [esp+516],1 \
    __asm je accepted119 \
    __asm cmp dword ptr [esp+516],2 \
    __asm je rejected119 \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [original] \
__asm accepted119: \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [yes] \
__asm rejected119: \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [no] \
}
ACTOR_ROUTE119(MacheteActorBodyBoundary119,MacheteActorBody119,g_actorBodyOriginal119,g_actorInclude119,g_actorReject119)
ACTOR_ROUTE119(MacheteActorSelectBoundary119,MacheteActorSelect119,g_actorSelectOriginal119,g_actorFinish119,g_actorObservationFinish121)
ACTOR_ROUTE119(MachetePawnRouteBoundary121,MachetePawnRoute121,g_pawnRouteOriginal121,g_pawnRouteCleanup121,g_pawnRouteCleanup121)
#undef ACTOR_ROUTE119
struct MacheteActorHook119 {UINT rva;BYTE signature[8];void* boundary;void** original;};
MacheteActorHook119 g_actorHooks119[]={
    {0x12d61d4,{0xf3,0x0f,0x10,0x44,0x24,0x38,0x33,0xc0},reinterpret_cast<void*>(MacheteActorSearchBoundary119),&g_actorSearchOriginal119},
    {0x173a684,{0x8b,0x47,0x0c,0x85,0xc0,0x0f,0x84,0xc7},reinterpret_cast<void*>(MacheteActorBodyBoundary119),&g_actorBodyOriginal119},
    {0x12d6244,{0x8b,0x44,0x24,0x18,0x8b,0x40,0x0c,0xf3},reinterpret_cast<void*>(MacheteActorSelectBoundary119),&g_actorSelectOriginal119},
    {0x12d69cf,{0x8b,0x4a,0x0c,0x8b,0x01,0x8b,0x40,0x28},reinterpret_cast<void*>(MacheteActorDispatchBoundary119),&g_actorDispatchOriginal119},
    {0x12d7fb5,{0x8b,0x0d,0x00,0xa4,0x82,0x12,0x8b,0x49},reinterpret_cast<void*>(MachetePawnRouteBoundary121),&g_pawnRouteOriginal121},
    {0x12d63a1,{0x8b,0x5d,0x0c,0x83,0xbb,0x1c,0x01,0x00},reinterpret_cast<void*>(MacheteWorldBoundary126),&g_macheteWorldOriginal126}
};
void MacheteActorSignature121(const MacheteActorHook119& hook,const BYTE* base,BYTE (&signature)[8])
{
    memcpy(signature,hook.signature,8);
    if(hook.rva==0x12d7fb5){const UINT global=reinterpret_cast<UINT>(base)+0x282a400;memcpy(signature+2,&global,4);}
}
bool InstallMacheteActors119(BYTE* base)
{
    for(const auto& h:g_actorHooks119){BYTE signature[8];MacheteActorSignature121(h,base,signature);if(memcmp(base+h.rva,signature,8))return false;}
    g_pawnRouteCleanup121=reinterpret_cast<UINT>(base)+0x12d7ffd;
    g_actorObservationFinish121=reinterpret_cast<UINT>(base)+0x12d6a42;
    g_actorInclude119=reinterpret_cast<UINT>(base)+0x173a819;g_actorReject119=reinterpret_cast<UINT>(base)+0x173a856;g_actorFinish119=reinterpret_cast<UINT>(base)+0x12d68c6;
    for(const auto& h:g_actorHooks119)if(MH_CreateHook(base+h.rva,h.boundary,h.original)!=MH_OK)return false;
    for(const auto& h:g_actorHooks119)if(MH_EnableHook(base+h.rva)!=MH_OK)return false;
    return true;
}
void RemoveMacheteActors119(BYTE* base)
{
    for(const auto& h:g_actorHooks119){MH_DisableHook(base+h.rva);MH_RemoveHook(base+h.rva);}
}
