// BZ19 Bizon (bison.xbg, profile 9, rifle family 7): rigid like the A2000. Native EMPTY reload captured in weapons4-102884
// (work/bz19_clips.py -> bz19_clips.inl): magazine out -> in (transfer) -> the LEFT hand pulls the left-side charging handle
// back and lets it go. FRAME +Y is the muzzle (the silencer sits at y +0.459): the handle rests FORWARD (y 0.158,
// kBz19ChargeIdleFromFrame) and is pulled back to y 0.076 (the clip file's "Rest" constant), 8.1 cm along FRAME -Y like
// every other rifle. (The clip file's own comments call these the other way round.)
#include "bz19_clips.inl"
const Mat4 kBz19HandFromFrame=RifleMat12(kBz19RightHandFromFrame);
const Mat4 kBz19SupportHandFromFrame=RifleMat12(kBz19LeftSupportFromFrame);
const Mat4 kBz19MagazineFromHand=RifleMat12(kBz19ClipFromLeftWrist);
const Mat4 kBz19ClipRest=RifleMat12(kBz19ClipRestFromFrame);
const Mat4 kBz19PartRest0=RifleMat12(kBz19ChargeIdleFromFrame); // charging handle at rest (forward)
const Mat4 kBz19PartRest1=RifleMat12(kBz19FlapRestFromFrame);   // magazine latch flap
const double kBz19ChargeTravelM=kBz19ChargeTravel;              // 0.0815 m
// Jason's left-hand pull: from the handle leaving rest (t 2.7108) to fully back (t 2.8179, frame 12558).
const double kBz19PullStart=kBz19EmptyChargeReleaseFwdAt,kBz19PullBack=2.8179;
Mat4 Bz19ChargeHandInit(){int k=0;for(int i=0;i<kBz19ClipEmptyReloadCount;++i)if(kBz19ClipEmptyReload[i].t<=kBz19PullStart)k=i;return RifleMat12(kBz19ClipEmptyReload[k].leftHand);}
const Mat4 kBz19ChargeHand=Bz19ChargeHandInit();
