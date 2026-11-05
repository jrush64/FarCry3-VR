// Visual1911 left holding fit. The existing native gun root and its captured
// right-hand grip stay authoritative. Reflect the hand across the gun's centre
// plane and its bind X axis: two reflections preserve a proper skin transform.
// Inventory, aiming, ownership, transfer timing and physical reloads are untouched.
void Left1911Role306(float* pose)
{
 const auto held=fc3hands303::ReadPresentation314();
 if(!pose||!held.enabled||!held.engaged||!held.active||held.empty||held.hand!=1||held.nativeHoldingSide!=0||g_weaponMechanicProfile!=20||g_frame-g_weaponMechanicFrame>2||g_freeRightEpoch!=g_armPoseEpoch)return;
 Mat4 inverse={};if(!Invert(kPistolHandFromFrame,inverse))return;
 //303 compensates the right palm onto the physical left palm. The left
 //1911 grasp is on the opposite face of the handle. Offset the logical gun
 //attachment by exactly that lateral difference before every consumer uses
 //it, keeping the final fitted left palm at the actual tracked physical palm.
 const Mat4 localRoot=Multiply(g_freeRightFromController,inverse);
 const V3 rightPalm=Apply(kPistolHandFromFrame,{.6781,.0519,1.0387});
 const double dx=2*rightPalm.x;
 const double delta[3]={localRoot.m[0]*dx,localRoot.m[4]*dx,localRoot.m[8]*dx};
 for(int row=0;row<3;++row)for(int col=0;col<3;++col)pose[row*4+3]+=float(pose[row*4+col]*delta[col]);
}
bool Left1911Holding305(const PhysicalArmScope303& scope,int side,Mat4& desired)
{
 const auto held=fc3hands303::ReadPresentation314();
 if(!scope.active||side!=1||!held.active||held.empty||held.hand!=1||held.nativeHoldingSide!=0||g_weaponMechanicProfile!=20||MacheteHeld())return false;
 // Arm drawing temporarily restores anatomical controller ordering. Read the
 // gun root using the exact logical poses saved by that scope, then restore it.
 float physical[2][16];memcpy(physical,g_handBody,sizeof(physical));const bool validRight=g_handRightValid,validLeft=g_handLeftValid;
 memcpy(g_handBody,scope.oldBody,sizeof(g_handBody));g_handRightValid=scope.oldValid[0];g_handLeftValid=scope.oldValid[1];Mat4 root={},grip={};
 const bool valid=ControllerCurrentRoot124(root,grip);
 memcpy(g_handBody,physical,sizeof(g_handBody));g_handRightValid=validRight;g_handLeftValid=validLeft;if(!valid)return false;
 //306: Kimber FRAME is centred laterally (actual Shadow handle bounds
 //-12.77..+14.64mm).305 reflected at the right PALM, x=+31.97mm,
 //leaving the left palm on the right face of the handle. Reflect the hand
 //at the native weapon centre instead; the gun/controller pose is unchanged.
 Mat4 plane=HandIdentity(),bind=HandIdentity();plane.m[0]=bind.m[0]=-1;
 desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply(grip,bind))));
 for(double value:desired.m)if(!std::isfinite(value))return false;
 static UINT reported306=~0u;if(reported306!=g_armPoseEpoch){reported306=g_armPoseEpoch;Log("[HOLD306] Shadow/Kimber left grip active; native weapon centre plane, captured1911 fingers; epoch=%u",g_armPoseEpoch);}
 return true;
}
void Left1911Fingers305(float* bones)
{
 const Mat4 wrist=BoneMatrix(bones,2);Mat4 mirror=HandIdentity();mirror.m[0]=-1;
 for(int bone=3;bone<=19;++bone)
  StoreBone(bones,bone,Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(kPistolRightGripRelative,bone-3),mirror))));
}
