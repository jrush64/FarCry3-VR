// U100 LMG (u100.xbg, profile 36): rigid, like the ACE. Native reloads captured in weapons6-115332 (work/u100_clips.py -> u100_clips.inl):
// EMPTY: magazine out -> in (transfer) -> the LEFT hand pulls the top charging handle (part 0, 372 indices) back 0.12 m along FRAME -Y and lets go. PARTIAL: magazine swap only.
// ONE magazine draw (14706/2658); parts 1..4 never move; no attachment draws.
#include "u100_clips.inl"
const Mat4 kU100HandFromFrame=RifleMat12(kU100RightHandFromFrame);
const Mat4 kU100SupportHandFromFrame=RifleMat12(kU100LeftSupportFromFrame);
const Mat4 kU100MagazineFromHand=RifleMat12(kU100ClipFromLeftWrist);
const Mat4 kU100ClipRest=RifleMat12(kU100ClipRestFromFrame);
const Mat4 kU100PartRest0=RifleMat12(kU100PartRest0FromFrame),kU100PartRest1=RifleMat12(kU100PartRest1FromFrame),kU100PartRest2=RifleMat12(kU100PartRest2FromFrame),kU100PartRest3=RifleMat12(kU100PartRest3FromFrame),kU100PartRest4=RifleMat12(kU100PartRest4FromFrame);
// Start of the handle pull = the key just before kU100EmptyChargePullStartAt.
double U100PullStartInit(){double s=0;for(int i=0;i<kU100ClipEmptyReloadCount;++i)if(kU100ClipEmptyReload[i].t<kU100EmptyChargePullStartAt)s=kU100ClipEmptyReload[i].t;return s;}
const double kU100PullStart=U100PullStartInit(),kU100PullBack=kU100EmptyChargeBackAt;
Mat4 U100ChargeHandInit(){int k=0;for(int i=0;i<kU100ClipEmptyReloadCount;++i)if(kU100ClipEmptyReload[i].t<=kU100PullStart)k=i;return RifleMat12(kU100ClipEmptyReload[k].leftHand);}
const Mat4 kU100ChargeHand=U100ChargeHandInit();
