// MKG LMG (minimi.xbg, profile 22): rigid, box-fed. Native EMPTY reload captured in weapons6-115332 (work/mkg_clips.py -> mkg_clips.inl):
// belt slides, cover flap (part 1) opens, box out, box in (transfer), flap shuts, the LEFT hand pulls the charging handle (part 0, 702 indices) back 14 cm along FRAME -Y and lets go.
// The box is FOUR draws (16182/906, 17088/462, 17550/1200 share one matrix; 28518/1218 is the belt) - see work/mkg-report.md.
#include "mkg_clips.inl"
const Mat4 kMkgHandFromFrame=RifleMat12(kMkgRightHandFromFrame);
const Mat4 kMkgSupportHandFromFrame=RifleMat12(kMkgLeftSupportFromFrame);
const Mat4 kMkgMagazineFromHand=RifleMat12(kMkgClipFromLeftWrist);
const Mat4 kMkgClipRest=RifleMat12(kMkgClipRestFromFrame);
const Mat4 kMkgPartRest0=RifleMat12(kMkgPartRest0FromFrame),kMkgPartRest1=RifleMat12(kMkgPartRest1FromFrame),kMkgPartRest2=RifleMat12(kMkgPartRest2FromFrame),kMkgPartRest3=RifleMat12(kMkgPartRest3FromFrame),kMkgPartRest4=RifleMat12(kMkgPartRest4FromFrame);
const Mat4 kMkgCoverOpen=RifleMat12(kMkgCoverOpenFromFrame);
const Mat4 kMkgExtraStaticRest=RifleMat12(kMkgExtraStaticRestFromFrame),kMkgBeltRest=RifleMat12(kMkgBeltRestFromFrame);
// Start of the handle pull = the key just before kMkgEmptyChargePullStartAt.
double MkgPullStartInit(){double s=0;for(int i=0;i<kMkgClipEmptyReloadCount;++i)if(kMkgClipEmptyReload[i].t<kMkgEmptyChargePullStartAt)s=kMkgClipEmptyReload[i].t;return s;}
const double kMkgPullStart=MkgPullStartInit(),kMkgPullBack=kMkgEmptyChargeBackAt;
Mat4 MkgChargeHandInit(){int k=0;for(int i=0;i<kMkgClipEmptyReloadCount;++i)if(kMkgClipEmptyReload[i].t<=kMkgPullStart)k=i;return RifleMat12(kMkgClipEmptyReload[k].leftHand);}
const Mat4 kMkgChargeHand=MkgChargeHandInit();
