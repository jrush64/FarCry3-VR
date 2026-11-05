// KSV Vector (kriss.xbg, profile 21): SKINNED like the MP5 / MK16 (src/mp5.inl places its palette slots). Native reloads captured in
// weapons5-95632 (work/vector_clips.py -> vector_clips.inl): magazine out -> in (transfer) -> the LEFT hand pulls the charging handle
// (SLIDE_HANDLE swings 85 deg while the SLIDE/bolt goes 42.6 mm back along FRAME -Y; it then snaps home at once). No shots were captured.
// Four draws share one world: 0/3615 (ANCHOR slot 0, CLIP slot 1), 3615/10623 (ANCHOR 0, FRONTSIGHT 1, BACKSIGHT 2, SLIDE 3, SLIDE_HANDLE 4),
// 14238/3906 (ANCHOR 0, Wire_Jiggle 1, BACKSIGHT 2), 18144/5091 (ANCHOR 0).
// Mp5DrawSlots entries to add for profile 21 (Mp5Slots{anchor,clip,handle,bolt}): (0,3615)->{0,1,-1,-1}  (3615,10623)->{0,-1,4,3}  (14238,3906)->{0,-1,-1,-1}  (18144,5091)->{0,-1,-1,-1}
#include "vector_clips.inl"
const Mat4 kVectorHandFromFrame=RifleMat12(kVectorRightHandFromFrame);
const Mat4 kVectorSupportHandFromFrame=RifleMat12(kVectorLeftSupportFromFrame);
const Mat4 kVectorMagazineFromHand=RifleMat12(kVectorClipFromLeftWrist);
const Mat4 kVectorClipRest=RifleMat12(kVectorClipRestFromFrame);
const Mat4 kVectorHandleRest=RifleMat12(kVectorChargeRestFromFrame),kVectorHandleBack=RifleMat12(kVectorChargeBackFromFrame);
const Mat4 kVectorBoltRest=RifleMat12(kVectorBoltRestFromFrame),kVectorBoltBack=RifleMat12(kVectorBoltBackFromFrame);
const Mat4 kVectorChargeHand=RifleMat12(kVectorClipEmptyReload[0].leftHand); // flag only: the pull pose comes from the clip
