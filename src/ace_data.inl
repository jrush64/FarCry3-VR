// Galil ACE 52 (galilace52.xbg, profile 17): rigid rifle like the P416. Native reloads captured in weapons5-95632 (work/ace_clips.py -> ace_clips.inl):
// magazine catch opens -> magazine out -> in (transfer) -> the LEFT hand pulls the charging handle (part 0, 906 indices) back 6.6 cm along FRAME -Y and lets go.
// ONE magazine draw (15219/672); part 1 (19137) is the magazine catch (rotates while the magazine is out; nothing rides the handle); optic vb 98144 drawn BEFORE the FRAME.
#include "ace_clips.inl"
const Mat4 kAceHandFromFrame=RifleMat12(kAceRightHandFromFrame);
const Mat4 kAceSupportHandFromFrame=RifleMat12(kAceLeftSupportFromFrame);
const Mat4 kAceMagazineFromHand=RifleMat12(kAceClipFromLeftWrist);
const Mat4 kAceClipRest=RifleMat12(kAceClipRestFromFrame);
const Mat4 kAcePartRest0=RifleMat12(kAcePartRest0FromFrame),kAcePartRest1=RifleMat12(kAcePartRest1FromFrame),kAcePartRest2=RifleMat12(kAcePartRest2FromFrame);
const Mat4 kAceLatchOpen=RifleMat12(kAceLatchOpenFromFrame);
// Start of the handle pull = the key just before kAceEmptyChargePullStartAt.
double AcePullStartInit(){double s=0;for(int i=0;i<kAceClipEmptyReloadCount;++i)if(kAceClipEmptyReload[i].t<kAceEmptyChargePullStartAt)s=kAceClipEmptyReload[i].t;return s;}
const double kAcePullStart=AcePullStartInit(),kAcePullBack=kAceEmptyChargeBackAt;
Mat4 AceChargeHandInit(){int k=0;for(int i=0;i<kAceClipEmptyReloadCount;++i)if(kAceClipEmptyReload[i].t<=kAcePullStart)k=i;return RifleMat12(kAceClipEmptyReload[k].leftHand);}
const Mat4 kAceChargeHand=AceChargeHandInit();
