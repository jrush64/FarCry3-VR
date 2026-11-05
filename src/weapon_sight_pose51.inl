// A cached corrected world matrix belongs to the camera/aim basis which made
// it. Keep the established body pose, and express it in each current draw's
// camera instead. Callers already own/classify the weapon; this helper never
// guesses a sight from a draw count, changes its native relative transform, or
// predicts a root from another rendered frame.
bool RefreshWeaponDrawRoot51(ID3D11DeviceContext* context)
{
 auto& p=g_pistolReload;
 if(!context||p.rootFrame!=g_frame||p.rootEpoch!=g_armPoseEpoch||p.rootContext!=context)return false;
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);
 CameraSummary camera={};UINT bytes=sizeof(camera);Mat4 inverse={};
 const bool valid=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)&&camera.mainView&&Invert(HullView(camera),inverse);
 SafeRelease(cameraBuffer);
 if(!valid)return false;
 p.rootWorld=Multiply(inverse,Multiply(ArmBodyToCamera(),p.rootBody));
 return true;
}

// Recompute the rigid attachment move in this draw's space. The native FRAME
// matrix remains the same frame's original matrix; all relative piece/bone
// motion remains native, including the movable reload parts.
bool RefreshRifleAttachmentMove51(ID3D11DeviceContext* context,Mat4& move)
{
 if(!RefreshWeaponDrawRoot51(context))return false;
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);
 CameraSummary camera={};UINT bytes=sizeof(camera);
 const bool valid=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)&&camera.mainView;
 SafeRelease(cameraBuffer);Mat4 inverse={};
 if(!valid||!Invert(Multiply(HullView(camera),g_pistolReload.rootNative),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),g_pistolReload.rootBody),inverse);
 return true;
}
