// PKM LMG (pkm.xbg, profile 43 when pkm.xbg is appended to kProfiles): SKINNED like the MP5 / MK16 / Vector (src/mp5.inl places palette slots). Native reload captured in
// weapons6-115332 (work/pkm_clips.py -> pkm_clips.inl): old box off -> new box seated (t=1.73 s) -> FEED COVER opens (2.68 s) -> belt laid -> cover closes (4.61 s) -> transfer 4.59 s -> bolt charged -> End 6.44 s.
// Draws that share ONE world: 0/9801 (ANCHOR 0, HANDLE 1, EJECT_LID 2, TOP 3, LEFT_LID 4, RIGHT_LID 5), 9801/840 (ANCHOR 0, HANDLE 1), 10641/1020 (BOX 0), 11661/2736 (BOX 0),
// 14397/23700 (ANCHOR 0, HANDLE 1, SLIDE 2, TRIGGER 3, TOP 4, TRIPOD2 5, TRIPOD 6, BOX 7, BOX_LID 8), 38397/3528 (BULLET01..12 = slots 0..11). The BOX draws and the BELT have NO anchor slot: like MK16 they are their own anchor and follow the box.
// Mp5DrawSlots entries to add for the PKM profile (Mp5Slots{anchor,clip,handle,bolt}); clip = the BOX slot; handle (-1): NO physical charging, the game plays the cover/bolt by itself; bolt (-1):
//   (0,9801)->{0,-1,-1,-1}  (9801,840)->{0,-1,-1,-1}  (10641,1020)->{0,0,-1,-1}  (11661,2736)->{0,0,-1,-1}  (14397,23700)->{0,7,-1,-1}  (38397,3528)->{0,-1,-1,-1}  [38097/300 is degenerate: leave unmapped]
// CHILDREN OF THE BOX that must follow a re-placed box: BOX_LID (slot 8 of draw 14397, kPkmBoxLidFromBoxRest/Open) and the 12 belt BULLETs (draw 38397, kPkmBulletsFromBoxRest). Their palette = FRAME x BOXPOSE x childFromBox x inv(T(bind)).
#include "pkm_clips.inl"
const Mat4 kPkmHandFromFrame=RifleMat12(kPkmRightHandFromFrame);
const Mat4 kPkmSupportHandFromFrame=RifleMat12(kPkmLeftSupportFromFrame);
const Mat4 kPkmMagazineFromHand=RifleMat12(kPkmClipFromLeftWrist);
const Mat4 kPkmMagazineFromHandIn=RifleMat12(kPkmClipFromLeftWristIn);
const Mat4 kPkmClipRest=RifleMat12(kPkmClipRestFromFrame);
const Mat4 kPkmCoverRest=RifleMat12(kPkmChargeRestFromFrame),kPkmCoverOpen=RifleMat12(kPkmChargeOpenFromFrame);
const Mat4 kPkmBoxLidRest=RifleMat12(kPkmBoxLidRestFromFrame),kPkmBoxLidOpen=RifleMat12(kPkmBoxLidOpenFromFrame);
const Mat4 kPkmBoxLidFromBoxRestM=RifleMat12(kPkmBoxLidFromBoxRest),kPkmBoxLidFromBoxOpenM=RifleMat12(kPkmBoxLidFromBoxOpen);
const Mat4 kPkmSlideRest=RifleMat12(kPkmSlideRestFromFrame),kPkmSlideBack=RifleMat12(kPkmSlideBackFromFrame);
const Mat4 kPkmHandleRest=RifleMat12(kPkmHandleRestFromFrame);
const Mat4 kPkmLeftHandAtGrip=RifleMat12(kPkmLeftHandAtBoxGrip);
// Box in the LEFT hand (the thumb looked wrong): the game's carry-away pose holds the thumb straight out
// (thumb curl 3-15 deg over keys 18..90). Same carry pose, with the thumb (finger bones 0-2 = arm bones 3-5) of Jason's idle PKM
// grip, curled toward the palm (24 deg).
struct PkmCarryFingers{float f[204];PkmCarryFingers(){for(int i=0;i<204;++i)f[i]=i<36?kPkmLeftIdleFingers[i]:kPkmLeftClipFingers[i];}};
const PkmCarryFingers kPkmLeftCarry;
const Mat4 kPkmChargeHand=RifleMat12(kPkmClipEmptyReload[0].leftHand); // flag only (no physical charge): the first key's wrist
