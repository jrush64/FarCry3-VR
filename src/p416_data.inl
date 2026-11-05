// P416 (p416.xbg, profile 31, rifle family 8): rigid like the A2000. Native EMPTY reload captured in weapons4-102884
// (work/p416_clips.py -> p416_clips.inl): magazine out -> in (transfer) -> the LEFT hand pulls the charging handle ON TOP
// back 5.7 cm along FRAME -Y and lets go. Two magazine draws (26529/1248 + 27777/546); the handle latch (part 1) rides the
// handle and tips 16.5 degrees. The optic (vb 98144) is drawn BEFORE the FRAME in every frame (same-frame attachment rule).
#include "p416_clips.inl"
const Mat4 kP416HandFromFrame=RifleMat12(kP416RightHandFromFrame);
const Mat4 kP416SupportHandFromFrame=RifleMat12(kP416LeftSupportFromFrame);
const Mat4 kP416MagazineFromHand=RifleMat12(kP416ClipFromLeftWrist);
const Mat4 kP416ClipRest=RifleMat12(kP416ClipRestFromFrame);
const Mat4 kP416PartRest0=RifleMat12(kP416PartRest0FromFrame),kP416PartRest1=RifleMat12(kP416PartRest1FromFrame);
const Mat4 kP416PartRest2=RifleMat12(kP416PartRest2FromFrame),kP416PartRest3=RifleMat12(kP416PartRest3FromFrame),kP416PartRest4=RifleMat12(kP416PartRest4FromFrame);
const Mat4 kP416LatchBack=RifleMat12(kP416LatchBackFromFrame);
// Jason's left-hand pull: the capture jumps 50 mm between the last still key and the first moved one (59 ms gap), so the
// path starts at the key before kP416EmptyChargePullStartAt.
double P416PullStartInit(){double s=0;for(int i=0;i<kP416ClipEmptyReloadCount;++i)if(kP416ClipEmptyReload[i].t<kP416EmptyChargePullStartAt)s=kP416ClipEmptyReload[i].t;return s;}
const double kP416PullStart=P416PullStartInit(),kP416PullBack=kP416EmptyChargeBackAt;
Mat4 P416ChargeHandInit(){int k=0;for(int i=0;i<kP416ClipEmptyReloadCount;++i)if(kP416ClipEmptyReload[i].t<=kP416PullStart)k=i;return RifleMat12(kP416ClipEmptyReload[k].leftHand);}
const Mat4 kP416ChargeHand=P416ChargeHandInit();
