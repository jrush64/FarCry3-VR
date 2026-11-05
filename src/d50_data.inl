// Desert Eagle D50 (desert_eagle.xbg, profile 12, magazine family 10): a rigid PISTOL on the 1911 / 6P9 path, but its own
// skeleton (the 6P9 could reuse the 1911 numbers; the D50 cannot). Native EMPTY reload captured in weapons4-102884
// (work/d50_clips.py -> d50_clips.inl): slide locked back (5.6 cm) -> magazine out -> in (transfer) -> the LEFT hand pulls
// the slide fully back (7.0 cm along FRAME -Y) and lets it go. Rule: after an EMPTY reload the slide must be pulled
// back before it fires; a partial reload fires at once.
const Mat4 kD50HandFromFrameM=RifleMat12(kD50HandFromFrame);
const Mat4 kD50MagazineFromHandM=RifleMat12(kD50MagazineFromHand);
const Mat4 kD50ClipRestM=RifleMat12(kD50ClipRest),kD50SlideRestM=RifleMat12(kD50SlideRest);
// per-pistol selectors (1911 / 6P9 share the 1911 skeleton)
const Mat4& PistolHandFromFrameCur(){return D50Model()?kD50HandFromFrameM:kPistolHandFromFrame;}
const Mat4& PistolMagazineFromHandCur(){return D50Model()?kD50MagazineFromHandM:kPistolMagazineFromHand;}
const float* PistolMagazineGripCur(){return D50Model()?kD50MagazineGripRelative:kPistolGripRelative;}
const double kD50LockPull=.0562; // the empty slide lock, behind closed
