// SVD (svd.xbg, profile 35, rifle family 10 proposal): rigid sniper like the A2000/P416, vertex buffer offset 10160 (not 0). Native reloads captured in weapons5-95632
// (work/svd_clips.py -> svd_clips.inl): magazine out -> in (transfer) -> the LEFT hand racks the charging handle back 13 cm along FRAME -Y and lets go. The partial and
// the empty reload play the SAME clip. One magazine draw (25068/1794), no rider part, parts 1 and 2 never move. The zoom lens 27207 replaces the body at full LT zoom.
#include "svd_clips.inl"
const Mat4 kSvdHandFromFrame=RifleMat12(kSvdRightHandFromFrame);
const Mat4 kSvdSupportHandFromFrame=RifleMat12(kSvdLeftSupportFromFrame);
const Mat4 kSvdMagazineFromHand=RifleMat12(kSvdClipFromLeftWrist);
const Mat4 kSvdClipRest=RifleMat12(kSvdClipRestFromFrame);
const Mat4 kSvdPartRest0=RifleMat12(kSvdPartRest0FromFrame),kSvdPartRest1=RifleMat12(kSvdPartRest1FromFrame);
const Mat4 kSvdPartRest2=RifleMat12(kSvdPartRest2FromFrame);
const Mat4 kSvdScope=RifleMat12(kSvdScopeFromFrame);
// The capture jumps between the last still key and the first fast key, so the path starts at the key before kSvdEmptyChargePullStartAt.
double SvdPullStartInit(){double s=0;for(int i=0;i<kSvdClipEmptyReloadCount;++i)if(kSvdClipEmptyReload[i].t<kSvdEmptyChargePullStartAt)s=kSvdClipEmptyReload[i].t;return s;}
const double kSvdPullStart=SvdPullStartInit(),kSvdPullBack=kSvdEmptyChargeBackAt;
Mat4 SvdChargeHandInit(){int k=0;for(int i=0;i<kSvdClipEmptyReloadCount;++i)if(kSvdClipEmptyReload[i].t<=kSvdPullStart)k=i;return RifleMat12(kSvdClipEmptyReload[k].leftHand);}
const Mat4 kSvdChargeHand=SvdChargeHandInit();
