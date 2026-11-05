// Candidate event observation only. No sound is suppressed, delayed or replayed.
// The caller's PistolLocal authority check is required in addition to the
// pointer/generation checks below; unknown receiver actors remain labelled unknown.
SRWLOCK g_d50AudioLock170=SRWLOCK_INIT;
struct D50AudioContext170 {
 D50AudioRecord170 value;
 ULONGLONG updated=0,actorReferenceId=0;
 UINT actor=0,actorReference=0;
 bool active=false;
} g_d50AudioContext170;
D50AudioRecord170 g_d50AudioQueue170[64];
UINT g_d50AudioQueued170=0,g_d50AudioGeneration170=0;
D50AudioStats170 g_d50AudioStats170;

bool D50AudioStructure170(UINT state,UINT wrapper,UINT weapon,UINT& actor,UINT& reference,ULONGLONG& referenceId){
 actor=Word(state);reference=Word(actor,4);
 const UINT component=Word(actor,0x24);
 return actor&&reference&&component&&component<=0xffffffffu-0x718&&state==component+0x718&&
        weapon&&Word(wrapper,0x48)==weapon&&Read(reference,&referenceId,sizeof(referenceId));
}
void ObserveD50Audio170(UINT receiver,UINT index,UINT firstPerson,UINT event){
 const UINT sound=Word(event,0x18),type=Word(event,0x1c);
 if(Word(event)!=g_engine+0x1f4d618||type!=15||sound<0x173064||sound>0x173069)return;
 ULONGLONG local=0,receiverActor=0;UINT receiverReference=0;
 AcquireSRWLockShared(&g_manualLock143);const bool localValid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 const bool receiverValid=SoundActor156(receiver,receiverActor,receiverReference);
 const auto now=GetTickCount64();
 AcquireSRWLockExclusive(&g_d50AudioLock170);
 auto& c=g_d50AudioContext170;
 UINT actor=0,reference=0;ULONGLONG referenceId=0;
 const bool valid=c.active&&localValid&&local==c.value.owner&&now>=c.updated&&now-c.updated<=500&&
  now>=c.value.beginMs&&now-c.value.beginMs<=8000&&
  D50AudioStructure170(c.value.state,c.value.wrapper,c.value.weapon,actor,reference,referenceId)&&
  actor==c.actor&&reference==c.actorReference&&referenceId==c.actorReferenceId;
 if(!valid){if(g_d50AudioStats170.rejectedContext!=0xffffffffu)++g_d50AudioStats170.rejectedContext;ReleaseSRWLockExclusive(&g_d50AudioLock170);return;}
 auto r=c.value;r.wallMs=now;r.receiver=receiver;r.reference=receiverReference;r.receiverActor=receiverActor;
 r.sound=sound;r.type=type;r.index=index;r.firstPerson=firstPerson;
 // wrapper+4 is recorded as a candidate entity reference, not assumed actor ownership.
 r.weaponReference=Word(r.wrapper,4);const UINT entity=Word(r.weaponReference,12);
 if(entity)ReadAt(entity,8,&r.weaponEntity,sizeof(r.weaponEntity));
 r.receiverMatch=receiverValid&&receiverActor==local?1:
  receiverValid&&r.weaponEntity&&r.weaponEntity!=~0ULL&&receiverActor==r.weaponEntity?2:0;
 if(!r.receiverMatch&&g_d50AudioStats170.unknownReceiver!=0xffffffffu)++g_d50AudioStats170.unknownReceiver;
 if(g_d50AudioStats170.recorded<384&&g_d50AudioQueued170<64){
  g_d50AudioQueue170[g_d50AudioQueued170++]=r;++g_d50AudioStats170.recorded;
 }else if(g_d50AudioStats170.dropped!=0xffffffffu)++g_d50AudioStats170.dropped;
 ReleaseSRWLockExclusive(&g_d50AudioLock170);
}
