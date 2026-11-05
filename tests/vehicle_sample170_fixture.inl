UINT fixtureVehiclePawn170=0,fixtureVehicleComponent170=0,fixtureVehicleTable170=0,fixtureVehicleEntry170=0;
int fixtureVehicleQueries170=0;bool fixtureVehicleLose170=false;
UINT __cdecl VehiclePawn170(UINT lo,UINT hi){return lo==fixtureHandle[0]&&hi==fixtureHandle[1]?fixtureVehiclePawn170:0;}
UINT __cdecl VehicleComponent170(UINT pawn){++fixtureVehicleQueries170;return pawn==fixtureVehiclePawn170&&!(fixtureVehicleLose170&&fixtureVehicleQueries170%2==0)?fixtureVehicleComponent170:0;}
__declspec(naked) void VehicleComponentThiscall170(){
    __asm {
        push ecx
        call VehicleComponent170
        add esp,4
        ret
    }
}
UINT __stdcall VehicleEntry170(UINT table,UINT pawn,UINT* optional){return table==fixtureVehicleTable170&&pawn==fixtureVehiclePawn170&&!optional?fixtureVehicleEntry170:0;}
void TestVehicleSample170(){
    const auto oldSample=g_sample;const auto oldProbe=g_vehicleSample170;
    void* oldPawn=g_pawnFromHandle;void* oldComponent=g_vehicleSeatOf;
    const auto oldLookup=g_vehicleSeatLookup170;const bool oldBound=g_vehicleProbeBound170;const auto oldAt=g_vehicleProbeAt170;const auto oldProbePawn=g_vehicleProbePawn170;
    const bool oldDriver173=g_vehicleDriverBound173;
    UINT pawn[16]={},component[8]={},reference[8]={},entity[32]={},entry[32]={},table[8]={};
    UINT pawnReference173[8]={},pawnEntity173[32]={},pawnState173[400]={};
    pawn[1]=reinterpret_cast<UINT>(pawnReference173);pawn[9]=reinterpret_cast<UINT>(pawnState173);
    pawnReference173[3]=reinterpret_cast<UINT>(pawnEntity173);memcpy(pawnEntity173+2,fixtureHandle,8);
    float* pawnMatrix173=reinterpret_cast<float*>(pawnEntity173+8);pawnMatrix173[0]=pawnMatrix173[5]=pawnMatrix173[10]=1;
    float* local173=reinterpret_cast<float*>(pawnState173+0x368/4);local173[0]=.2f;local173[2]=.4f;
    float* world173=reinterpret_cast<float*>(pawnState173+0x584/4);world173[0]=.3f;world173[2]=.7f;
    g_vehicleDriverBound173=true;
    fixtureVehiclePawn170=reinterpret_cast<UINT>(pawn);fixtureVehicleComponent170=reinterpret_cast<UINT>(component);
    fixtureVehicleTable170=reinterpret_cast<UINT>(table);fixtureVehicleEntry170=reinterpret_cast<UINT>(entry);
    component[1]=reinterpret_cast<UINT>(reference);component[4]=fixtureVehicleTable170;component[5]=3;
    reference[3]=reinterpret_cast<UINT>(entity);entity[2]=123;entity[3]=456;entry[4]=2;
    float* transform=reinterpret_cast<float*>(entity+8);transform[0]=transform[5]=transform[10]=1;transform[12]=100;transform[13]=-20;transform[14]=3;
    g_pawnFromHandle=reinterpret_cast<void*>(VehiclePawn170);g_vehicleSeatOf=reinterpret_cast<void*>(VehicleComponentThiscall170);
    g_vehicleSeatLookup170=VehicleEntry170;g_vehicleProbeBound170=true;g_vehicleProbeAt170=0;g_vehicleSample170={};
    g_sample.valid=true;g_sample.vehicle=1;g_sample.seat=3;memcpy(g_sample.handle,fixtureHandle,8);g_sample.ms=testNow;
    const int beforeWrites=writes;
    SampleVehicle170(fixtureHandle,1,3,testNow);
    fc3aim::VehicleSample170 s;
    Check(fc3aim::GetVehicleSample170(s)&&s.identityValid&&s.seatValid&&s.basisValid&&s.actualSeat==2&&s.type==3&&
          s.vehicleId==(123ull|(456ull<<32))&&s.position[0]==100,"170 actual cdecl/thiscall/stdcall chain copies identity, distinct seat/type and matrix");
    Check(s.pawnBasisValid173&&s.lookValid173&&s.pawnBasis173[1][1]==1&&s.localLook173[2]==.4f&&s.worldLook173[2]==.7f,"173 same snapshot copies pawn basis and distinct native local/world angles");
    fc3aim::VehicleSample170 optional173;
    pawn[9]=0;SamplePawn173(fixtureVehiclePawn170,VehicleId170(fixtureHandle),optional173);
    Check(optional173.pawnBasisValid173&&!optional173.lookValid173,"173 absent state preserves independently valid pawn basis");pawn[9]=reinterpret_cast<UINT>(pawnState173);
    optional173={};++pawnEntity173[2];SamplePawn173(fixtureVehiclePawn170,VehicleId170(fixtureHandle),optional173);
    Check(!optional173.pawnBasisValid173&&!optional173.lookValid173,"173 wrong pawn entity identity refuses optional snapshot");--pawnEntity173[2];
    optional173={};pawn[9]=1;SamplePawn173(fixtureVehiclePawn170,VehicleId170(fixtureHandle),optional173);
    Check(!optional173.lookValid173,"173 inaccessible optional native state fails closed under SEH");pawn[9]=reinterpret_cast<UINT>(pawnState173);
    const int queries=fixtureVehicleQueries170;SampleVehicle170(fixtureHandle,1,3,testNow+20);
    Check(queries==fixtureVehicleQueries170,"170 native basis probe limited to10Hz");
    transform[0]=4;testNow+=100;g_sample.ms=testNow;SampleVehicle170(fixtureHandle,1,3,testNow);
    Check(fc3aim::GetVehicleSample170(s)&&s.identityValid&&!s.basisValid,"170 invalid basis preserves independently verified identity");
    transform[0]=1;fixtureVehicleEntry170=0;testNow+=100;g_sample.ms=testNow;SampleVehicle170(fixtureHandle,1,3,testNow);
    Check(fc3aim::GetVehicleSample170(s)&&!s.seatValid&&s.actualSeat==-1,"170 missing seat entry is not mistaken for seat zero");
    fixtureVehicleEntry170=reinterpret_cast<UINT>(entry);entry[4]=0;testNow+=100;g_sample.ms=testNow;SampleVehicle170(fixtureHandle,1,3,testNow);
    Check(fc3aim::GetVehicleSample170(s)&&s.seatValid&&s.actualSeat==0,"170 actual seat zero remains valid");
    fixtureVehicleQueries170=0;fixtureVehicleLose170=true;testNow+=100;g_sample.ms=testNow;SampleVehicle170(fixtureHandle,1,3,testNow);
    Check(!fc3aim::GetVehicleSample170(s),"170 vehicle component changes during read reject whole sample");
    const int failedQueries170=fixtureVehicleQueries170;
    for(int n=1;n<=19;++n)SampleVehicle170(fixtureHandle,1,3,testNow+n*5);
    Check(fixtureVehicleQueries170==failedQueries170,"170 failed ownership read keeps10Hz limit across repeated input polls");
    SampleVehicle170(fixtureHandle,1,3,testNow+100);
    Check(fixtureVehicleQueries170==failedQueries170+2,"170 failed read retries after bounded interval");
    ++fixtureHandle[0];SampleVehicle170(fixtureHandle,1,3,testNow+105);
    Check(fixtureVehicleQueries170==failedQueries170+4,"170 new pawn bypasses failed previous-owner cadence immediately");
    --fixtureHandle[0];
    fixtureVehicleLose170=false;testNow+=100;
    testNow+=100;g_sample.ms=testNow;SampleVehicle170(fixtureHandle,1,3,testNow);
    AcquireSRWLockExclusive(&g_nativeLock);const bool recursive=fc3aim::GetVehicleSample170(s);ReleaseSRWLockExclusive(&g_nativeLock);
    Check(!recursive,"170 copied observation refuses lock recursion");
    testNow+=251;Check(!fc3aim::GetVehicleSample170(s),"170 stale native observation cannot identify vehicle episode");
    g_sample.ms=testNow;g_vehicleProbeAt170=0;SampleVehicle170(fixtureHandle,1,3,testNow);++g_sample.handle[0];
    Check(!fc3aim::GetVehicleSample170(s),"170 respawn/local identity mismatch rejected");memcpy(g_sample.handle,fixtureHandle,8);
    SampleVehicle170(fixtureHandle,0,-1,testNow);Check(!fc3aim::GetVehicleSample170(s),"170 exit invalidates without cadence delay");
    Check(writes==beforeWrites,"170 diagnostics never call native camera setter");
    g_sample=oldSample;g_vehicleSample170=oldProbe;g_pawnFromHandle=oldPawn;g_vehicleSeatOf=oldComponent;
    g_vehicleSeatLookup170=oldLookup;g_vehicleProbeBound170=oldBound;g_vehicleProbeAt170=oldAt;g_vehicleProbePawn170=oldProbePawn;
    g_vehicleDriverBound173=oldDriver173;
    puts("PASS: vehicle170 native ABI, identity/seat/type separation, bounded reads, matrix validation, zero seat, lifecycle, contention, no camera writes");
}
