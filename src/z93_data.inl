// Z93 (m93.xbg, profile 23): rigid bolt-action rifle, vb 388232 stride 32 vertex offset 10760, ib 85386.
// Body = FRAME (6 draws 2058/5772/8148/14148/15459/16437 share one world). Parts: 0 = bolt (37260), 1 = 36630 (rigid with the bolt), 2/3 = swinging pair 27540/31752, 4 = 35964, (5 = 40560); clip = 24462 (the magazine the LEFT hand removes), clipB = 26808. The RIGHT hand works the bolt after every shot.
#include "z93_clips.inl"
#include "z93_settled.inl"
const Mat4 kZ93HandFromFrame=RifleMat12(kZ93RightHandFromFrame);
const Mat4 kZ93SupportHandFromFrame=RifleMat12(kZ93LeftSupportFromFrame);
const Mat4 kZ93MagazineFromHand=RifleMat12(kZ93ClipSettledFromLeftWrist); // the settled carry, not the pull-out moment (55 deg off)
const Mat4 kZ93ClipRest=RifleMat12(kZ93ClipRestFromFrame);
const Mat4 kZ93ClipBRest=RifleMat12(kZ93ClipBRestFromFrame);
const Mat4 kZ93PartRest0=RifleMat12(kZ93PartRest0FromFrame),kZ93PartRest1=RifleMat12(kZ93PartRest1FromFrame),kZ93PartRest2=RifleMat12(kZ93PartRest2FromFrame),kZ93PartRest3=RifleMat12(kZ93PartRest3FromFrame),kZ93PartRest4=RifleMat12(kZ93PartRest4FromFrame),kZ93PartRest5=RifleMat12(kZ93PartRest5FromFrame);
const Mat4 kZ93BoltOpen=RifleMat12(kZ93BoltOpenFromFrame),kZ93BoltReloadOpen=RifleMat12(kZ93BoltReloadOpenFromFrame);
const Mat4 kZ93RightHandAtBolt=RifleMat12(kZ93RightHandAtBoltFromFrame);
const Mat4 kZ93Scope=RifleMat12(kZ93ScopeFromFrame),kZ93Scope2=RifleMat12(kZ93Scope2FromFrame);
