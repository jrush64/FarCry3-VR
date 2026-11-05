// MP5 (mp5.xbg, profile 28, rifle family 5): a SKINNED weapon. FRAME = draw world x palette[ANCHOR]; the magazine (bone 10),
// bolt (bone 11) and charging handle (bone 12) are palette slots, not rigid draws (src/mp5.inl places them). Native reloads
// captured in weapons3-3384 (empty) and weapons3-famas-99068 (partial): work/mp5_clips.py -> mp5_clips.inl. The native empty
// reload pulls the handle BEFORE the magazine swap; the rule (as on the A2000): insert, THEN pull the handle, only when empty.
#include "mp5_clips.inl"
const Mat4 kMp5HandFromFrame=RifleMat12(kMp5RightHandFromFrame);
const Mat4 kMp5SupportHandFromFrame=RifleMat12(kMp5LeftSupportFromFrame);
const Mat4 kMp5MagazineFromHand=RifleMat12(kMp5ClipFromLeftWrist);
const Mat4 kMp5ClipRest=RifleMat12(kMp5ClipRestFromFrame);
const Mat4 kMp5HandleRest=RifleMat12(kMp5HandleRestFromFrame),kMp5HandleBack=RifleMat12(kMp5HandleBackFromFrame);
const Mat4 kMp5BoltRest=RifleMat12(kMp5BoltRestFromFrame),kMp5BoltBack=RifleMat12(kMp5BoltBackFromFrame);
// The native MP5 reload never grips its handle (the hand only passes near it), so the hand-on-handle pose is Jason's own
// support hand moved so the palm sits just under the handle (palm at idle (-.044,.162,-.013) -> (-.01,.262,.05)).
Mat4 Mp5ChargeHandInit(){Mat4 m=RifleMat12(kMp5LeftSupportFromFrame);m.m[3]+=.034;m.m[7]+=.10;m.m[11]+=.063;return m;}
const Mat4 kMp5ChargeHand=Mp5ChargeHandInit(); // flag only: the pull pose comes from RifleChargeClipSample
// Jason's hand relative to a charging handle he actually grips: the F1 key with its handle fully back (wrist and handle
// both rel FRAME) -> inv(handle) x hand. Applied to the MP5 handle's own path.
Mat4 Mp5HandFromHandleInit(){int k=0;for(int i=0;i<kFamasClipEmptyReloadCount;++i)if(kFamasClipEmptyReload[i].t<=kFamasEmptySlideBackAt)k=i;Mat4 inv={};Invert(RifleMat12(kFamasClipEmptyReload[k].slide),inv);return Multiply(inv,RifleMat12(kFamasClipEmptyReload[k].leftHand));}
const Mat4 kMp5HandFromHandle=Mp5HandFromHandleInit();
const double kMp5ChargeTravel=kMp5HandleTravel; // 0.0658 m along FRAME -Y (plus a 50 degree swing)
const float kMp5PortDefault[3]={-0.0577f,0.1273f,-0.1606f};  // left palm at the native seat frame (work/analyze_mp5_famas*.py)
const float kMp5ChargeDefault[3]={-0.03f,0.25f,0.07f};        // beside the charging handle (the native hand never grips it: pick-your-own box)
