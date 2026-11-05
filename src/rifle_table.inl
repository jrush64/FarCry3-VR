// Two-handed magazine rifles share the AK physical-reload path (instant native reload at the
// request seam, physical removal/insertion, support foregrip). Every rifle-specific number lives here.
// AK: PID35744 captures. A2000 and STG-90: PID56072 (captures/weapons2-56072, work/prepare_rifles.py).
// MP5 (skinned: magazine / handle / bolt are palette slots, src/mp5.inl) and FAMAS F1 (rigid, bullpup):
// weapons3-3384 + weapons3-famas-99068 (work/mp5_clips.py, work/famas_clips.py).
#include "a2000_data.inl"
#include "stg90_data.inl"
#include "famas_data.inl"
#include "mp5_data.inl"
#include "bz19_data.inl"
#include "p416_data.inl"
#include "mk16_data.inl"
#include "vector_data.inl"
#include "ace_data.inl"
#include "svd_data.inl"
#include "r700_data.inl"
#include "u100_data.inl"
#include "mkg_data.inl"
#include "z93_data.inl"
#include "pkm_data.inl"
struct RifleTable
{
 LONG profile;int family;const wchar_t* name;
 UINT vb,ib,stride,offset;bool skinned;UINT frame[6][2];UINT clip[2],clipB[2];UINT part[6][2];
 const Mat4 *hand,*support,*magazine,*clipRest;const Mat4* partRest[6];
 const float *rightGrip,*supportGrip,*magazineGrip,*chargeGrip;
 const Mat4* chargeHand; // left wrist rel FRAME holding the charging handle at rest (nullptr: wrist stays on the controller)
 double chargeRestY;   // <-100: use partRest[0] (native handle rest on FRAME-local Y)
 double chargeTravel;  // native handle rest-to-rear travel; the physical pull is scaled to it
 bool emptyNeedsCharge;// rule: empty reload needs a physical handle pull before firing
 int ringPart;         // AK jiggle ring held at rest during reload, -1 none
 const Mat4* riderBack; // part 1 rides the handle pull to this pose (P416 latch), nullptr none
 // weapons 6 (MKG box = four draws): clipC = another box draw with the box's own matrix; clipD = a piece (the belt) that is part of
 // the box in the hand but has its own rest on the gun while the box is seated (clipDRest, rel FRAME).
 UINT clipC[2]={0,0},clipD[2]={0,0};const Mat4* clipDRest=nullptr;
};
const RifleTable kRifles[16]={
 {8,2,L"AK",250208,51150,32,0,false,{{0,1056},{1056,17421},{18477,1308},{19785,1818},{0,0},{0,0}},{21603,2766},{0,0},{{24369,276},{24645,750},{25395,180},{0,0},{0,0}},
  &kAkHandFromFrame,&kAkSupportHandFromFrame,&kAkMagazineFromHand,&kAkClipRest,{&kAkBoltRest,&kAkRingRest,&kAkAccessoryRest,nullptr,nullptr},
  kAkRightGripRelative,kAkSupportGripRelative,kAkMagazineGripRelative,kAkChargingGripRelative,nullptr,.3180705,.05,false,1},
 {7,3,L"A2000",214976,45228,32,0,false,{{0,8754},{8754,2736},{11490,8184},{0,0},{0,0},{0,0}},{19674,1392},{0,0},{{21066,528},{21594,768},{22362,252},{0,0},{0,0}},
  &kA2000HandFromFrame,&kA2000SupportHandFromFrame,&kA2000MagazineFromHand,&kA2000ClipRest,{&kA2000PartRest0,&kA2000PartRest1,&kA2000PartRest2,nullptr,nullptr},
  kA2000RightGripRelative,kA2000SupportGripRelative,kA2000MagazineGripRelative,kA2000ChargingGripRelative,nullptr,-1000,kA2000ChargeTravel,true,-1},
 {41,4,L"STG-90",304128,52560,32,0,false,{{0,54},{54,2688},{2742,2784},{5526,5256},{10782,13122},{0,0}},{23904,2112},{0,0},{{26016,156},{26172,108},{0,0},{0,0},{0,0}},
  &kStg90HandFromFrame,&kStg90SupportHandFromFrame,&kStg90MagazineFromHand,&kStg90ClipRest,{&kStg90PartRest0,&kStg90PartRest1,nullptr,nullptr,nullptr},
  kStg90RightGripRelative,kStg90SupportGripRelative,kStg90MagazineGripRelative,kAkChargingGripRelative,nullptr,-1000,.05,false,-1},
 // MP5: the three skinned draws are the FRAME (palette anchor); no rigid clip / part draws.
 {28,5,L"MP5",306360,55536,40,0,true,{{0,4149},{4149,8085},{12234,15534},{0,0},{0,0},{0,0}},{0,0},{0,0},{{0,0},{0,0},{0,0},{0,0},{0,0}},
  &kMp5HandFromFrame,&kMp5SupportHandFromFrame,&kMp5MagazineFromHand,&kMp5ClipRest,{&kMp5HandleRest,nullptr,nullptr,nullptr,nullptr},
  kMp5RightIdleFingers,kMp5LeftIdleFingers,kMp5LeftClipFingers,kMp5LeftHandleFingers,nullptr,-1000,kMp5ChargeTravel,true,-1}, // empty reload requires the physical handle pull
 // FAMAS F1: two magazine draws (one magazine), the middle charging handle = part 0, bipods and flip sights fixed.
 {14,6,L"F1",216704,39378,32,0,false,{{0,1332},{1332,3396},{4728,6528},{11256,3924},{0,0},{0,0}},{15180,135},{15315,618},{{15933,582},{16515,642},{17157,642},{17799,870},{18669,1020}},
  &kFamasHandFromFrame,&kFamasSupportHandFromFrameMat,&kFamasMagazineFromHand,&kFamasClipRest,{&kFamasPartRest0,&kFamasPartRest1,&kFamasPartRest2,&kFamasPartRest3,&kFamasPartRest4},
  kFamasRightIdleFingers,kFamasLeftIdleFingers,kFamasLeftClipFingers,kFamasLeftSlideFingers,&kFamasChargeHand,-1000,kFamasChargeTravel,true,-1}, // EMPTY reload = pull the middle handle, Jason's own hand path
 // BZ19 Bizon: left-side charging handle (part 0, rests forward, pulled 8.1 cm), magazine latch flap (part 1).
 {9,7,L"BZ19",259616,48300,32,0,false,{{0,4875},{4875,7521},{12396,2178},{14574,5199},{0,0},{0,0}},{19773,3738},{0,0},{{23511,519},{24030,120},{0,0},{0,0},{0,0}},
  &kBz19HandFromFrame,&kBz19SupportHandFromFrame,&kBz19MagazineFromHand,&kBz19ClipRest,{&kBz19PartRest0,&kBz19PartRest1,nullptr,nullptr,nullptr},
  kBz19RightIdleFingers,kBz19LeftIdleFingers,kBz19LeftClipFingers,kBz19LeftChargeFingers,&kBz19ChargeHand,-1000,kBz19ChargeTravelM,true,-1,nullptr},
 // P416: top charging handle (part 0, 5.7 cm), its latch rides it (part 1), three static parts; two magazine draws.
 {31,8,L"P416",341536,65556,32,0,false,{{0,2853},{2853,13368},{16221,3804},{20025,2250},{22275,4254},{0,0}},{26529,1248},{27777,546},{{28524,936},{29460,288},{28323,201},{29748,2160},{31908,870}},
  &kP416HandFromFrame,&kP416SupportHandFromFrame,&kP416MagazineFromHand,&kP416ClipRest,{&kP416PartRest0,&kP416PartRest1,&kP416PartRest2,&kP416PartRest3,&kP416PartRest4},
  kP416RightIdleFingers,kP416LeftIdleFingers,kP416LeftClipFingers,kP416LeftChargeFingers,&kP416ChargeHand,-1000,kP416ChargeTravel,true,-1,&kP416LatchBack},
 // MK16: skinned (mp5.inl): five draws = FRAME; magazine / handle / bolt are palette slots.
 {25,9,L"MK16",291640,49818,40,0,true,{{0,5412},{5412,2076},{7488,2808},{10296,732},{11028,13881},{0,0}},{0,0},{0,0},{{0,0},{0,0},{0,0},{0,0},{0,0}},
  &kMk16HandFromFrame,&kMk16SupportHandFromFrame,&kMk16MagazineFromHand,&kMk16ClipRest,{&kMk16HandleRest,nullptr,nullptr,nullptr,nullptr},
  kMk16RightIdleFingers,kMk16LeftIdleFingers,kMk16LeftClipFingers,kMk16LeftChargeFingers,&kMk16ChargeHand,-1000,kMk16ChargeTravel,true,-1,nullptr},
 // weapons 5 (captures/weapons5-95632). KSV Vector: skinned (mp5.inl), four draws = FRAME; CLIP / SLIDE_HANDLE / SLIDE are palette slots.
 {21,11,L"Vector",311800,46470,40,0,true,{{0,3615},{3615,10623},{14238,3906},{18144,5091},{0,0},{0,0}},{0,0},{0,0},{{0,0},{0,0},{0,0},{0,0},{0,0}},
  &kVectorHandFromFrame,&kVectorSupportHandFromFrame,&kVectorMagazineFromHand,&kVectorClipRest,{&kVectorHandleRest,nullptr,nullptr,nullptr,nullptr},
  kVectorRightIdleFingers,kVectorLeftIdleFingers,kVectorLeftClipFingers,kVectorLeftChargeFingers,&kVectorChargeHand,-1000,kVectorChargeTravel,true,-1,nullptr},
 // Galil ACE52: rigid, top charging handle (part 0, 6.6 cm), magazine catch (part 1) and a static part; optic vb 98144 drawn before the FRAME.
 {17,12,L"ACE",210720,38826,32,0,false,{{0,4707},{4707,5211},{9918,5301},{0,0},{0,0},{0,0}},{15219,672},{0,0},{{15891,906},{19137,276},{16797,2340},{0,0},{0,0}},
  &kAceHandFromFrame,&kAceSupportHandFromFrame,&kAceMagazineFromHand,&kAceClipRest,{&kAcePartRest0,&kAcePartRest1,&kAcePartRest2,nullptr,nullptr},
  kAceRightIdleFingers,kAceLeftIdleFingers,kAceLeftClipFingers,kAceLeftChargeFingers,&kAceChargeHand,-1000,kAceChargeTravel,true,-1,nullptr},
 // SVD: rigid, every draw binds the vertex buffer at offset 10160; left-side bolt handle (part 0, 13 cm), two static parts.
 {35,13,L"SVD",268464,59526,32,10160,false,{{1968,978},{3033,3909},{6942,2448},{9390,4320},{13710,11358},{0,0}},{25068,1794},{0,0},{{26862,243},{27105,102},{29031,732},{0,0},{0,0}},
  &kSvdHandFromFrame,&kSvdSupportHandFromFrame,&kSvdMagazineFromHand,&kSvdClipRest,{&kSvdPartRest0,&kSvdPartRest1,&kSvdPartRest2,nullptr,nullptr},
  kSvdRightIdleFingers,kSvdLeftIdleFingers,kSvdLeftClipFingers,kSvdLeftChargeFingers,&kSvdChargeHand,-1000,kSvdChargeTravel,true,-1,nullptr},
 // R700 (r700.inl): rigid bolt-action, vertex offset 9560; the loose block (1434) is the "magazine", the bolt is part 0 (RIGHT grip cycles
 // it after a shot), part 1 (30363) slides with the block in the native reload. No left charging handle: emptyNeedsCharge false.
 {2,14,L"R700",368728,62190,32,9560,false,{{2880,1764},{4644,4098},{8742,13419},{0,0},{0,0},{0,0}},{1434,966},{0,0},{{28971,1386},{30363,732},{0,0},{0,0},{0,0}},
  &kR700HandFromFrame,&kR700SupportHandFromFrame,&kR700MagazineFromHand,&kR700ClipRest,{&kR700PartRest0,&kR700PartRest1,nullptr,nullptr,nullptr},
  kR700RightIdleFingers,kR700LeftIdleFingers,kR700LeftClipFingers,kR700LeftClipFingers,nullptr,-1000,.07,false,-1,nullptr},
 // weapons 6 (captures/weapons6-115332). U100: top charging handle (part 0, 12 cm); parts 1+5 are two contiguous static draws.
 {36,15,L"U100",247392,45756,32,0,false,{{0,2406},{2406,12300},{0,0},{0,0},{0,0},{0,0}},{14706,2658},{0,0},{{17364,372},{17736,972},{19104,546},{19650,546},{20196,2682},{18708,396}},
  &kU100HandFromFrame,&kU100SupportHandFromFrame,&kU100MagazineFromHand,&kU100ClipRest,{&kU100PartRest0,&kU100PartRest1,&kU100PartRest2,&kU100PartRest3,&kU100PartRest4,&kU100PartRest1},
  kU100RightIdleFingers,kU100LeftIdleFingers,kU100LeftClipFingers,kU100LeftChargeFingers,&kU100ChargeHand,-1000,kU100ChargeTravel,true,-1,nullptr},
 // MKG (minimi.xbg): box = 16182 + 17088 + 17550 (one matrix) + the belt 28518 (own rest while seated); top handle part 0 (14 cm),
 // cover flap part 1 (stays shut: the box seats instantly), part 3 loose (keeps its native motion), parts 4/5 static.
 {22,16,L"MKG",309952,59472,32,0,false,{{0,1584},{1584,612},{2196,192},{2388,10752},{23916,984},{24900,3090}},{16182,906},{17088,462},{{13140,702},{27990,528},{15018,252},{15270,912},{14082,936},{13842,240}},
  &kMkgHandFromFrame,&kMkgSupportHandFromFrame,&kMkgMagazineFromHand,&kMkgClipRest,{&kMkgPartRest0,&kMkgPartRest1,&kMkgPartRest2,&kMkgPartRest3,&kMkgPartRest4,&kMkgExtraStaticRest},
  kMkgRightIdleFingers,kMkgLeftIdleFingers,kMkgLeftClipFingers,kMkgLeftChargeFingers,&kMkgChargeHand,-1000,kMkgChargeTravel,true,-1,nullptr,{17550,1200},{28518,1218},&kMkgBeltRest},
 // Z93 (m93.xbg, r700.inl family): vertex offset 10760; bolt = parts 0+1, swinging pair 2/3 pinned, 4/5 static; the follower (26808)
 // rises during every shot natively, so it is the clipD piece with its own seated rest. Nothing to pull on the left.
 {23,17,L"Z93",388232,85386,32,10760,false,{{2058,3714},{5772,2376},{8148,6000},{14148,1311},{15459,978},{16437,8025}},{24462,2346},{0,0},{{37260,3300},{36630,630},{27540,4212},{31752,4212},{35964,666},{40560,207}},
  &kZ93HandFromFrame,&kZ93SupportHandFromFrame,&kZ93MagazineFromHand,&kZ93ClipRest,{&kZ93PartRest0,&kZ93PartRest1,&kZ93PartRest2,&kZ93PartRest3,&kZ93PartRest4,&kZ93PartRest5},
  kZ93RightIdleFingers,kZ93LeftIdleFingers,kZ93LeftSettledFingers,kZ93LeftSettledFingers,nullptr,-1000,.15,false,-1,nullptr,{0,0},{26808,732},&kZ93ClipBRest},
 // PKM (pkm.inl): skinned; six draws share one world (three carry the ANCHOR slot); the box is a palette slot; the game's own reload
 // plays after the box is seated by hand. Nothing to pull.
 {43,18,L"PKM",636120,83850,40,0,true,{{0,9801},{9801,840},{10641,1020},{11661,2736},{14397,23700},{38397,3528}},{0,0},{0,0},{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  &kPkmHandFromFrame,&kPkmSupportHandFromFrame,&kPkmMagazineFromHand,&kPkmClipRest,{&kPkmCoverRest,nullptr,nullptr,nullptr,nullptr,nullptr},
  kPkmRightIdleFingers,kPkmLeftIdleFingers,kPkmLeftCarry.f,kPkmLeftClipInFingers,nullptr,-1000,0,false,-1,nullptr},
};
// Shared assault-rifle contact/cue scope. MS16 uses the native mk16 asset.
bool AssaultRifleProfile234(LONG p){return p==8||p==41||p==14||p==31||p==25||p==17;}
const RifleTable& RifleForProfile(LONG profile){for(const auto& r:kRifles)if(r.profile==profile)return r;return kRifles[0];}
const RifleTable& RifleForFamily(int family){for(const auto& r:kRifles)if(r.family==family)return r;return kRifles[0];}
const RifleTable& CurrentRifle(){return RifleForProfile(g_weaponMechanicProfile);}
double RifleChargeRestY(){const auto& r=CurrentRifle();return r.chargeRestY>-100?r.chargeRestY:r.partRest[0]->m[7];}
bool RiflePart(const UINT (&part)[2],UINT start,UINT count){return part[1]&&part[0]==start&&part[1]==count;}
bool RifleClip(const RifleTable& r,UINT start,UINT count){return RiflePart(r.clip,start,count)||RiflePart(r.clipB,start,count)||RiflePart(r.clipC,start,count)||RiflePart(r.clipD,start,count);}
int RiflePartIndex(UINT start,UINT count){const auto& r=CurrentRifle();for(int i=0;i<6;++i)if(RiflePart(r.part[i],start,count))return i;return -1;}
template<class G> bool RifleMain(const RifleTable& r,const G& d){return d.vertexBytes==r.vb&&d.indexBytes==r.ib&&d.stride==r.stride&&d.offset==r.offset&&d.format==57;}
