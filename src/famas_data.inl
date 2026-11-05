// FAMAS F1 (famas.xbg, profile 14, rifle family 6): rigid parts like the A2000, bullpup (the magazine sits BEHIND the grip,
// FRAME y -0.134). Native empty reload captured in weapons3-famas-99068 (work/famas_clips.py -> famas_clips.inl): magazine
// out -> in (transfer) -> the middle charging handle (draw 15933) pulled 13 cm along FRAME -Y by the LEFT hand -> released.
// Rule: a partial reload needs no handle pull.
#include "famas_clips.inl"
Mat4 RifleMat12(const float* v){Mat4 m={};for(int i=0;i<12;++i)m.m[i]=v[i];m.m[15]=1;return m;}
Mat4 RifleRestAt(double x,double y,double z){Mat4 m={};m.m[0]=m.m[5]=m.m[10]=m.m[15]=1;m.m[3]=x;m.m[7]=y;m.m[11]=z;return m;}
const Mat4 kFamasHandFromFrame=RifleMat12(kFamasRightHandFromFrame);
const Mat4 kFamasSupportHandFromFrameMat=RifleMat12(kFamasLeftSupportFromFrame);
const Mat4 kFamasMagazineFromHand=RifleMat12(kFamasClipFromLeftWrist);
const Mat4 kFamasClipRest=RifleMat12(kFamasClipRestFromFrame);
const Mat4 kFamasPartRest0=RifleMat12(kFamasSlideRestFromFrame);       // charging handle (draw 15933/582)
const Mat4 kFamasPartRest1=RifleRestAt(-.0208,.2429,.0762);            // BIPOD_L 16515/642 (idle rest, never moves)
const Mat4 kFamasPartRest2=RifleRestAt(.0195,.2429,.0762);             // BIPOD_R 17157/642
const Mat4 kFamasPartRest3=RifleRestAt(0,.103,.1583);                  // FRONTSIGHT_FLIP 17799/870
const Mat4 kFamasPartRest4=RifleRestAt(0,-.0144,.1583);                // BACKSIGHT_FLIP 18669/1020
// Jason's left wrist on the middle charging handle (the clip key at the pull start, handle at rest; the left hand
// needs the correct pose during the pull).
Mat4 FamasChargeHandInit(){int k=0;for(int i=0;i<kFamasClipEmptyReloadCount;++i)if(kFamasClipEmptyReload[i].t<=kFamasEmptySlidePullStartAt)k=i;return RifleMat12(kFamasClipEmptyReload[k].leftHand);}
const Mat4 kFamasChargeHand=FamasChargeHandInit();
const double kFamasChargeTravel=kFamasSlideTravel;                     // 0.1299 m along FRAME -Y
const float kFamasPortDefault[3]={kFamasLeftPalmAtSeat[0],kFamasLeftPalmAtSeat[1],kFamasLeftPalmAtSeat[2]};
const float kFamasChargeDefault[3]={kFamasLeftPalmAtHandlePull[0],kFamasLeftPalmAtHandlePull[1],kFamasLeftPalmAtHandlePull[2]};
