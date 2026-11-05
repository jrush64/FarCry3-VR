// MK16 (mk16.xbg, profile 25, rifle family 9): SKINNED like the MP5 (src/mp5.inl places its palette slots). Native EMPTY
// reload captured in weapons4-102884 (work/mk16_clips.py -> mk16_clips.inl): magazine out -> in (transfer) -> the LEFT hand
// pulls the charging handle (BOLTSLIDE) 9.5 cm back along FRAME -Y; the BOLT swings 70 degrees about Y with it.
// Five draws share one world: 0/5412 (CLIP slot 0, ANCHOR slot 2), 5412/2076 and 7488/2808 (ANCHOR slot 0), 10296/732
// (BULLET slot 0, child of CLIP: same skin matrix), 11028/13881 (BOLTSLIDE 0, BOLT 1, MAGRELEASE 2, ANCHOR 3).
#include "mk16_clips.inl"
const Mat4 kMk16HandFromFrame=RifleMat12(kMk16RightHandFromFrame);
const Mat4 kMk16SupportHandFromFrame=RifleMat12(kMk16LeftSupportFromFrame);
const Mat4 kMk16MagazineFromHand=RifleMat12(kMk16ClipFromLeftWrist);
const Mat4 kMk16ClipRest=RifleMat12(kMk16ClipRestFromFrame);
const Mat4 kMk16HandleRest=RifleMat12(kMk16ChargeRestFromFrame),kMk16HandleBack=RifleMat12(kMk16ChargeBackFromFrame);
const Mat4 kMk16BoltRest=RifleMat12(kMk16BoltRestFromFrame),kMk16BoltBack=RifleMat12(kMk16BoltBackFromFrame);
const Mat4 kMk16ChargeHand=RifleMat12(kMk16ClipEmptyReload[0].leftHand); // flag only: the pull pose comes from the clip
