// Native pose investigation only. Do not label the magazine-grasp fallback as a rack frame.
LONG g_pistolRackSourceFrame[2]={-100000,-100000};UINT g_pistolRackSourceCount=0,g_pistolRackSourceEpoch=0;
void ObservePistolRackSource(ID3D11DeviceContext* context,int side,const float* original)
{
    if(!PistolHandling()||!g_animation.enabled||g_pistolMenu.open)return;
    const auto& p=g_pistolReload;
    if(!p.cycle&&!p.pending&&GetTickCount64()>g_pistolRackCaptureUntil){g_pistolRackSourceCount=0;return;}
    if(g_pistolRackSourceEpoch!=g_armPoseEpoch){g_pistolRackSourceCount=0;g_pistolRackSourceEpoch=g_armPoseEpoch;}
    if(g_pistolRackSourceCount>=960||g_pistolRackSourceFrame[side]==g_frame||g_frame-p.rootFrame>1)return;
    ID3D11Buffer* globalsBuffer=nullptr;context->VSGetConstantBuffers(0,1,&globalsBuffer);float globals[88]={};const UINT bytes=ReadCopy(globalsBuffer,globals,sizeof(globals));SafeRelease(globalsBuffer);
    Mat4 inverse={};if(bytes<48||!Invert(p.rootNative,inverse))return;
    const Mat4 relative=Multiply(inverse,Multiply(HullWorld(globals),BoneMatrix(original,2)));
    AnimationRecord record;record.kind=19;record.side=side;AnimationStamp(record);
    record.rawBytes=25*12*sizeof(float);memcpy(record.raw,original,record.rawBytes);
    memcpy(record.globals,globals,sizeof(globals));
    for(int i=0;i<16;++i){record.finalPose[i]=static_cast<float>(relative.m[i]);record.finalPose[16+i]=static_cast<float>(p.rootNative.m[i]);}
    record.finalBytes=32*sizeof(float);record.stateWords[0]=p.rootFrame;record.stateWords[1]=p.nativeSlideFrame;
    record.inputs[0]=static_cast<float>(p.nativeSlidePull);record.flags=(p.pending?1:0)|(p.cycle?2:0)|(p.rackHeld?4:0);
    g_pistolRackSourceFrame[side]=g_frame;++g_pistolRackSourceCount;
    g_animation.sample=true;g_animation.until=GetTickCount64()+10000;AnimationStore(record);
    if(g_pistolRackSourceCount<=2)Log("[PISTOL RACK SOURCE] native %s arm and slide source paired; raw frame versus physical override recorded",side==1?"left":"right");
}
