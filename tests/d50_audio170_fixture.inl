volatile UINT g_audioCalls170=0;
void __fastcall AudioOriginal170(void*,void*,UINT,UINT,UINT){++g_audioCalls170;}
void TestD50Audio170(){
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"170 audio fixture");
 const UINT base=reinterpret_cast<UINT>(page),actor=base+128,component=base+512,state=component+0x718,
  wrapper=base+32,weapon=base+2500,reference=base+2600,entity=base+2700,receiver=base+2900,event=base+3000;
 auto put=[](UINT p,UINT v){*reinterpret_cast<UINT*>(p)=v;};
 const auto oldOriginal=g_soundOriginal156;const auto oldReader=g_identity143;const auto oldLocal=fixtureLocal143;
 g_soundOriginal156=reinterpret_cast<void*>(AudioOriginal170);fixtureLocal143=0x1111222233334444ULL;SetIdentityReader143(ReadLocal143);
 put(actor+0x24,component);put(actor+4,reference);put(state,actor);put(wrapper+0x48,weapon);
 *reinterpret_cast<ULONGLONG*>(reference)=fixtureLocal143;put(reference+12,entity);*reinterpret_cast<ULONGLONG*>(entity+8)=fixtureLocal143;
 put(receiver-8,reference);put(event,g_engine+0x1f4d618);put(event+0x18,0x173068);put(event+0x1c,15);
 g_d50AudioContext170={};g_d50AudioStats170={};g_d50AudioQueued170=g_d50AudioGeneration170=g_audioCalls170=0;
 auto play=[&](){PinSoundGate156(reinterpret_cast<void*>(receiver),nullptr,7,1,event);};
 D50AudioRecord170 rows[64];
 play();Check(g_audioCalls170==1&&DrainD50Audio170(rows,64)==0&&D50AudioStatus170().rejectedContext==1,"170 candidate without verified reload passes unchanged and rejects recording");
 PublishD50Audio170(1,state,wrapper,weapon,7,5);play();
 Check(g_audioCalls170==2&&DrainD50Audio170(rows,64)==1&&rows[0].generation==1&&rows[0].receiverMatch==1&&rows[0].owner==fixtureLocal143&&rows[0].sound==0x173068&&rows[0].stage==1,"170 exact local reload correlates sound without suppressing it");
 put(event+0x18,0x1234);play();Check(g_audioCalls170==3&&!DrainD50Audio170(rows,64),"170 unrelated ID remains native and unrecorded");put(event+0x18,0x173068);
 put(event+0x1c,8);play();Check(g_audioCalls170==4&&!DrainD50Audio170(rows,64),"170 unrelated type remains native and unrecorded");put(event+0x1c,15);
 PublishD50Audio170(2,state,wrapper,weapon,7,7);play();Check(DrainD50Audio170(rows,64)==1&&rows[0].stage==2&&rows[0].generation==1,"170 transfer retains reload generation");
 PublishD50Audio170(3,state,wrapper,weapon,7,3);PublishD50Audio170(4,state,wrapper,weapon,7,35);play();Check(DrainD50Audio170(rows,64)==1&&rows[0].stage==3&&rows[0].flags==35,"170 heartbeat retains native end stage");
 *reinterpret_cast<ULONGLONG*>(entity+8)=0x5555666677778888ULL;play();Check(DrainD50Audio170(rows,64)==1&&rows[0].receiverMatch==0,"170 unknown receiver explicitly labelled research observation, not verified owner");
 put(wrapper+4,reference);play();Check(DrainD50Audio170(rows,64)==1&&rows[0].receiverMatch==2&&rows[0].weaponEntity==0x5555666677778888ULL,"170 wrapper entity correspondence recorded distinctly from local actor");
 *reinterpret_cast<ULONGLONG*>(entity+8)=fixtureLocal143;
 const UINT calls=g_audioCalls170;put(wrapper+0x48,weapon+4);play();Check(g_audioCalls170==calls+1&&!DrainD50Audio170(rows,64),"170 weapon pointer reuse rejects recording but never audio");put(wrapper+0x48,weapon);
 *reinterpret_cast<ULONGLONG*>(reference)+=1;play();Check(!DrainD50Audio170(rows,64),"170 actor reference reuse rejected");*reinterpret_cast<ULONGLONG*>(reference)=fixtureLocal143;
 PublishD50Audio170(4,state,wrapper,weapon,8,5);play();Check(!DrainD50Audio170(rows,64),"170 epoch change invalidates old generation");
 PublishD50Audio170(1,state,wrapper,weapon,8,5);play();Check(DrainD50Audio170(rows,64)==1&&rows[0].generation==2,"170 new begin creates distinct generation");
 ++fixtureLocal143;play();Check(!DrainD50Audio170(rows,64),"170 changed local identity rejected");--fixtureLocal143;
 g_d50AudioContext170.updated=GetTickCount64()-501;play();Check(!DrainD50Audio170(rows,64),"170 stale heartbeat rejected");
 PublishD50Audio170(1,state,wrapper,weapon,8,5);g_d50AudioContext170.value.beginMs=GetTickCount64()-8001;PublishD50Audio170(4,state,wrapper,weapon,8,5);play();Check(!DrainD50Audio170(rows,64),"170 heartbeat cannot extend generation past eight seconds");
 PublishD50Audio170(1,state,wrapper,weapon,8,5);g_d50AudioStats170={};g_d50AudioQueued170=0;
 for(UINT i=0;i<70;++i)play();Check(g_d50AudioQueued170==64&&D50AudioStatus170().dropped==6,"170 callback queue bounded at64");
 Check(DrainD50Audio170(rows,1)==1&&g_d50AudioQueued170==63&&DrainD50Audio170(rows,64)==63,"170 bounded partial drain preserves remainder");
 for(UINT i=0;i<8;++i){for(UINT j=0;j<64;++j)play();DrainD50Audio170(rows,64);}
 Check(D50AudioStatus170().recorded==384,"170 session diagnostic capped at384 records");
 PublishD50Audio170(0,0,0,0,0,0);Check(!g_d50AudioContext170.active,"170 explicit cancel invalidates generation");
 g_soundOriginal156=oldOriginal;SetIdentityReader143(oldReader);fixtureLocal143=oldLocal;
 g_d50AudioContext170={};g_d50AudioStats170={};g_d50AudioQueued170=g_d50AudioGeneration170=0;
 VirtualFree(page,0,MEM_RELEASE);puts("PASS:170 D50 audio read-only passthrough, exact reload generations, receiver classification, stale/identity/epoch guards and bounded diagnostics");
}
