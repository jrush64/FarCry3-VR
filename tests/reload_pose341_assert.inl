#pragma once
// Compare actual uploaded wrist/finger bones with the captured native grasp,
// reflected at the visible part. No production pose helper supplies expected data.
void CheckReloadPose341(const float* skin,const Mat4& root,const Mat4& captured,const float* fingers,int source,bool centrePlane,const Mat4* part=nullptr){
 const auto world=Multiply(ArmBodyToCamera(),root);
 const auto contact=Apply(captured,{source?-.6781:.6781,.0519,1.0387});
 const double plane=centrePlane?0:2*contact.x;Mat4 partInverse={};if(part)Check(Invert(*part,partInverse),"341 actual captured operating part invertible");
 for(int b=2;b<=19;++b){
  const auto native=b==2?captured:Multiply(captured,BoneMatrix(fingers,b-3));
  const auto actual=BoneMatrix(skin,b);V3 expectedLocal={plane-native.m[3],native.m[7],native.m[11]};
  if(part){auto local=Apply(partInverse,{native.m[3],native.m[7],native.m[11]});local.x=-local.x;expectedLocal=Apply(*part,local);}
  const V3 expected=Apply(world,expectedLocal);
  const double positionError=Length(Sub({actual.m[3],actual.m[7],actual.m[11]},expected));
  if(positionError>=.003)printf("341 pose mismatch profile%d clip%d bone%d source%d err%.6f actual%.5f/%.5f/%.5f expected%.5f/%.5f/%.5f\n",g_weaponMechanicProfile,g_lever.clip,b,source,positionError,actual.m[3],actual.m[7],actual.m[11],expected.x,expected.y,expected.z);
  Check(positionError<.003,"341 uploaded wrist/finger joints fit captured grasp on visible reload part");
  for(int c=0;c<3;++c){
   const double sign=c==0?-1:1;V3 local={-native.m[c]*sign,native.m[4+c]*sign,native.m[8+c]*sign};
   if(part){const V3 origin={native.m[3],native.m[7],native.m[11]};auto endpoint=Apply(partInverse,Add(origin,{native.m[c]*sign,native.m[4+c]*sign,native.m[8+c]*sign}));endpoint.x=-endpoint.x;local=Sub(Apply(*part,endpoint),expectedLocal);}
   const V3 expectedAxis={world.m[0]*local.x+world.m[1]*local.y+world.m[2]*local.z,world.m[4]*local.x+world.m[5]*local.y+world.m[6]*local.z,world.m[8]*local.x+world.m[9]*local.y+world.m[10]*local.z};
   Check(Length(Sub({actual.m[c],actual.m[4+c],actual.m[8+c]},expectedAxis))<.004,"341 uploaded wrist/fingers retain authored grasp orientation after anatomical mirror");
  }
 }
 if(part){Mat4 inverse={};Check(Invert(captured,inverse),"341 captured operating wrist invertible");const auto bind=SidearmMirror315();const auto nativeAttachment=Multiply(inverse,*part);const auto oppositeAttachment=Multiply(bind,Multiply(nativeAttachment,bind));const auto actualPart=Multiply(BoneMatrix(skin,2),oppositeAttachment),expectedPart=Multiply(world,*part);for(int k=0;k<16;++k)Check(fabs(actualPart.m[k]-expectedPart.m[k])<.004,"341 mirrored operating-hand attachment reconstructs exact native shell/lever frame");}
}
