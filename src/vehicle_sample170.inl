// Included inside aim_probe.cpp's private namespace. Observational only: never writes native memory.
fc3aim::VehicleSample170 g_vehicleSample170;
UINT g_vehicleEntity352=0;
bool g_vehicleProbeBound170=false;
bool g_vehicleDriverBound173=false;
ULONGLONG g_vehicleProbeAt170=0;
unsigned long long g_vehicleProbePawn170=0; // last attempted owner, independent of sample success
using VehicleSeatLookup170=UINT (__stdcall*)(UINT,UINT,UINT*);
VehicleSeatLookup170 g_vehicleSeatLookup170=nullptr;
unsigned long long VehicleId170(const UINT* handle){return static_cast<unsigned long long>(handle[0])|(static_cast<unsigned long long>(handle[1])<<32);}
bool VehicleBasisValid170(const fc3aim::VehicleSample170& s)
{
    for(int i=0;i<3;++i){
        if(!std::isfinite(s.position[i]))return false;
        double norm=0;for(int j=0;j<3;++j){if(!std::isfinite(s.basis[i][j]))return false;norm+=double(s.basis[i][j])*s.basis[i][j];}
        if(fabs(norm-1)>.08)return false;
        for(int k=0;k<i;++k){double dot=0;for(int j=0;j<3;++j)dot+=double(s.basis[i][j])*s.basis[k][j];if(fabs(dot)>.04)return false;}
    }
    return true;
}
bool VehicleProbeSignatures170(const BYTE* base)
{
    auto exact=[&](UINT r,const BYTE* bytes,size_t n){return memcmp(base+r,bytes,n)==0;};
    auto call=[&](UINT site,UINT target){if(base[site]!=0xe8)return false;int rel=0;memcpy(&rel,base+site+1,4);return site+5+rel==target;};
    const BYTE owner[]={0x8b,0x40,4,0x8b,0x40,0xc,0x8b,0x48,8,0x8b,0x50,0xc};
    const BYTE matrix[]={0x8b,0x46,0xc,0x83,0xc0,0x20};
    const BYTE entry[]={0x8b,0x40,0x10};const BYTE ret12[]={0xc2,0x0c,0};
    const BYTE table[]={0x8b,0x50,0x10,0x6a,0,0x56,0x52};
    const BYTE lookup[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x14};
    return exact(0x1987ff2,owner,sizeof(owner))&&exact(0x17d2144,matrix,sizeof(matrix))&&
        call(0x1987fcd,0x657c40)&&call(0x1987fe9,0x13770a0)&&call(0x17d2151,0x64400)&&
        exact(0x66f7d9,table,sizeof(table))&&call(0x66f7e0,0x2d3360)&&exact(0x66f7e9,entry,sizeof(entry))&&
        exact(0x2d3360,lookup,sizeof(lookup))&&exact(0x2d3451,ret12,sizeof(ret12))&&exact(0x2d3463,ret12,sizeof(ret12));
}
bool VehicleDriverSignatures173(const BYTE* base)
{
    const BYTE state[]={0x8b,0x43,0x0c,0x8b,0x70,0x24};
    const BYTE matrix[]={0x8b,0x43,0x0c,0x8b,0x48,0x04,0x8b,0x41,0x0c,0x83,0xc0,0x20};
    const BYTE local[]={0xf3,0x0f,0x10,0x8e,0x68,0x03,0,0};
    const BYTE world[]={0xf3,0x0f,0x11,0x90,0x84,0x05,0,0};
    return !memcmp(base+0x12c0070,state,sizeof(state))&&!memcmp(base+0x12c075d,matrix,sizeof(matrix))&&
        !memcmp(base+0x12c0251,local,sizeof(local))&&!memcmp(base+0x12c0825,world,sizeof(world));
}
void SamplePawn173(UINT pawn,unsigned long long expected,fc3aim::VehicleSample170& out)
{
    // Optional failure must not invalidate independently copied vehicle identity/basis.
    __try {
        const UINT reference=*reinterpret_cast<const UINT*>(pawn+4);
        const UINT entity=reference?*reinterpret_cast<const UINT*>(reference+0xc):0;
        const UINT state=*reinterpret_cast<const UINT*>(pawn+0x24);
        if(!entity||VehicleId170(reinterpret_cast<const UINT*>(entity+8))!=expected)return;
        fc3aim::VehicleSample170 copy;
        for(int axis=0;axis<3;++axis)memcpy(copy.basis[axis],reinterpret_cast<const void*>(entity+0x20+axis*0x10),12);
        const bool basisValid=VehicleBasisValid170(copy);
        float local[3]={},world[3]={};bool lookValid=state!=0;
        if(state){memcpy(local,reinterpret_cast<const void*>(state+0x368),12);memcpy(world,reinterpret_cast<const void*>(state+0x584),12);
            for(int i=0;i<3;++i)lookValid=lookValid&&std::isfinite(local[i])&&std::isfinite(world[i]);}
        if(*reinterpret_cast<const UINT*>(pawn+4)!=reference||*reinterpret_cast<const UINT*>(reference+0xc)!=entity||
           VehicleId170(reinterpret_cast<const UINT*>(entity+8))!=expected||*reinterpret_cast<const UINT*>(pawn+0x24)!=state)return;
        if(basisValid){memcpy(out.pawnBasis173,copy.basis,sizeof(copy.basis));out.pawnBasisValid173=true;}
        if(lookValid){memcpy(out.localLook173,local,12);memcpy(out.worldLook173,world,12);out.lookValid173=true;}
    } __except(EXCEPTION_EXECUTE_HANDLER){} // unknown optional state remains invalid
}
void SampleVehicle170(const UINT* handle,int vehicle,int type,ULONGLONG now)
{
    const auto pawnId=VehicleId170(handle);
    if(vehicle!=1||!g_vehicleProbeBound170||!g_vehicleSeatLookup170||!g_pawnFromHandle||!g_vehicleSeatOf){g_vehicleSample170={};g_vehicleProbeAt170=0;g_vehicleProbePawn170=0;return;}
    if(g_vehicleProbePawn170!=pawnId){g_vehicleSample170={};g_vehicleProbeAt170=0;g_vehicleProbePawn170=pawnId;}
    if(g_vehicleProbeAt170&&now-g_vehicleProbeAt170<100)return;
    g_vehicleProbeAt170=now;
    fc3aim::VehicleSample170 next;next.ms=now;next.pawnId=pawnId;next.type=type;
    void* fromHandle=g_pawnFromHandle;void* vehicleOf=g_vehicleSeatOf;
    UINT h0=handle[0],h1=handle[1],pawn=0,component=0;
    __try {
        __asm {
            push h1
            push h0
            call fromHandle
            add esp,8
            mov pawn,eax
        }
        if(!pawn){g_vehicleSample170={};return;}
        __asm {
            mov ecx,pawn
            call vehicleOf
            mov component,eax
        }
        if(!component){g_vehicleSample170={};return;}
        const UINT reference=*reinterpret_cast<const UINT*>(component+4);
        const UINT entity=reference?*reinterpret_cast<const UINT*>(reference+0xc):0;
        if(!entity){g_vehicleSample170={};return;}
        const UINT* id=reinterpret_cast<const UINT*>(entity+8);
        next.vehicleId=VehicleId170(id);
        if(next.vehicleId==~0ull||next.vehicleId==0||*reinterpret_cast<const int*>(component+0x14)!=type){g_vehicleSample170={};return;}
        next.identityValid=true;
        const UINT table=*reinterpret_cast<const UINT*>(component+0x10);
        const UINT entry=table?g_vehicleSeatLookup170(table,pawn,nullptr):0;
        if(entry){next.actualSeat=*reinterpret_cast<const int*>(entry+0x10);next.seatValid=next.actualSeat>=0&&next.actualSeat<64;}
        for(int axis=0;axis<3;++axis)memcpy(next.basis[axis],reinterpret_cast<const void*>(entity+0x20+axis*0x10),12);
        memcpy(next.position,reinterpret_cast<const void*>(entity+0x50),12);
        next.basisValid=VehicleBasisValid170(next);
        if(g_vehicleDriverBound173)SamplePawn173(pawn,pawnId,next);
        // Reject an ownership change while resolving the seat/table and copying the transform.
        UINT again=0;
        __asm {
            mov ecx,pawn
            call vehicleOf
            mov again,eax
        }
        if(again!=component||*reinterpret_cast<const UINT*>(component+4)!=reference||
           *reinterpret_cast<const UINT*>(reference+0xc)!=entity||VehicleId170(id)!=next.vehicleId){g_vehicleSample170={};return;}
        g_vehicleEntity352=entity;g_vehicleSample170=next;
    } __except(EXCEPTION_EXECUTE_HANDLER){g_vehicleSample170={};}
}
