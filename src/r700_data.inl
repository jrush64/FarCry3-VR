// Remington R700 (700sniperrifle.xbg, profile 2): rigid bolt-action rifle, vb 368728 stride 32 vertex offset 9560, ib 62190.
// Body = FRAME (3 draws 2880/4644/8742 share one world). Parts: 0 = bolt (28971), 1 = 30363, clip = 1434 (the block the LEFT hand removes). The RIGHT hand works the bolt after every shot.
#include "r700_clips.inl"
const Mat4 kR700HandFromFrame=RifleMat12(kR700RightHandFromFrame);
const Mat4 kR700SupportHandFromFrame=RifleMat12(kR700LeftSupportFromFrame);
const Mat4 kR700MagazineFromHand=RifleMat12(kR700ClipFromLeftWrist);
const Mat4 kR700ClipRest=RifleMat12(kR700ClipRestFromFrame);
const Mat4 kR700ClipBRest=RifleMat12(kR700ClipBRestFromFrame);
const Mat4 kR700PartRest0=RifleMat12(kR700PartRest0FromFrame),kR700PartRest1=RifleMat12(kR700PartRest1FromFrame),kR700PartRest2=RifleMat12(kR700PartRest2FromFrame);
const Mat4 kR700BoltOpen=RifleMat12(kR700BoltOpenFromFrame),kR700BoltReloadOpen=RifleMat12(kR700BoltReloadOpenFromFrame);
const Mat4 kR700RightHandAtBolt=RifleMat12(kR700RightHandAtBoltFromFrame);
const Mat4 kR700Scope=RifleMat12(kR700ScopeFromFrame);
