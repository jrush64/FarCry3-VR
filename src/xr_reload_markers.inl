// Debug contact points are OpenXR quads in the same reference space as tracking.
XrSwapchain g_reloadMarkerChain=XR_NULL_HANDLE;
ID3D11Texture2D* g_reloadMarkerImages[8]={};UINT g_reloadMarkerCount=0;
ID3D11Texture2D* g_reloadMarkerUpload=nullptr;
XrPosef g_reloadMarkerPose[21]={};bool g_reloadMarkerVisible[21]={};XrExtent2Df g_reloadBoxSize[6]={};
bool g_weaponReloadGroup=true,g_twoHandGroup=true,g_guideInsertOpen=false;
ULONGLONG g_doorHighlightAt363=0;unsigned g_doorHighlightReference363=0;bool g_doorHighlightHeld363=false;XrExtent2Df g_doorHighlightSize363[2]={};
bool GuideMarkerVisible(int i){if(i<0||i>=21||!g_reloadMarkerVisible[i])return false;if(i>=19){const auto now=GetTickCount64();return !fc3pad::ThrowableInputActive363()&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&now>=g_doorHighlightAt363&&now-g_doorHighlightAt363<150&&g_doorHighlightReference363==fc3xr::ReferenceGeneration286();}return i<10?g_weaponReloadGroup:g_twoHandGroup;}
bool ReloadBoxFaceVisible(){for(int i=2;i<8;++i)if(GuideMarkerVisible(i))return true;return false;}
bool AnyGuideVisible(){for(int i=0;i<21;++i)if(GuideMarkerVisible(i))return true;return false;}
float g_reloadHandDiameter=.014f;bool g_reloadHandSphere=false;
// Two-hand grab gizmos (all weapons): quad 10 = the grab point on the gun, 11-13 = the red palm sphere.
float g_grabPointDiameter=.2f,g_grabPalmDiameter=.1f;bool g_grabInReach=false;ULONGLONG g_grabMarkersAt=0;
float g_reloadMarkerRadius=0;int g_reloadMarkerColor=-1,g_reloadMarkerState=0;bool g_reloadMarkerFailed=false;
bool ReloadMarkerResources(ID3D11Texture2D* frame,int color)
{
    if(g_reloadMarkerFailed)return false;
    ID3D11Device* device=nullptr;frame->GetDevice(&device);
    if(!g_reloadMarkerUpload)
    {
        XrSwapchainCreateInfo info={XR_TYPE_SWAPCHAIN_CREATE_INFO};info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        info.format=g_format;info.sampleCount=1;info.width=448;info.height=64;info.faceCount=info.arraySize=info.mipCount=1;
        if(!XR_SUCCEEDED(p_xrCreateSwapchain(g_session,&info,&g_reloadMarkerChain))){device->Release();g_reloadMarkerFailed=true;return false;}
        UINT count=0;p_xrEnumerateSwapchainImages(g_reloadMarkerChain,0,&count,nullptr);
        if(count==0||count>8){p_xrDestroySwapchain(g_reloadMarkerChain);g_reloadMarkerChain=XR_NULL_HANDLE;device->Release();g_reloadMarkerFailed=true;return false;}
        XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};
        if(!XR_SUCCEEDED(p_xrEnumerateSwapchainImages(g_reloadMarkerChain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images)))){device->Release();g_reloadMarkerFailed=true;return false;}
        for(UINT i=0;i<count;++i)g_reloadMarkerImages[i]=images[i].texture;g_reloadMarkerCount=count;
        D3D11_TEXTURE2D_DESC td={};td.Width=448;td.Height=64;td.MipLevels=td.ArraySize=1;td.Format=g_format;td.SampleDesc.Count=1;
        if(FAILED(device->CreateTexture2D(&td,nullptr,&g_reloadMarkerUpload))){device->Release();g_reloadMarkerFailed=true;return false;}
        Log("[XR RELOAD] loading ring and shell contact dot resources ready");
    }
    if(color!=g_reloadMarkerColor)
    {
        // regions: 0 port/box, 1 reload hand, 2 grab palm, 3 grab point, 4 grab point in reach
        static unsigned pixels[448*64];std::memset(pixels,0,sizeof(pixels));
        for(int y=0;y<64;++y)for(int x=0;x<448;++x)
        {
            const int cx=x%64,region=x/64;const float dx=cx-31.5f,dy=y-31.5f,d=std::sqrt(dx*dx+dy*dy);
            const bool on=region==0?((color&2)?(cx>=3&&cx<=60&&y>=3&&y<=60&&(cx<=5||cx>=58||y<=5||y>=58)):(d>27&&d<31)):
                region==1?((color&4)?(d>27&&d<31):d<22):(region==3||region==4||region>=5)?(cx>=3&&cx<=60&&y>=3&&y<=60&&(cx<=5||cx>=58||y<=5||y>=58)):(d>27&&d<31);
            if(on)
            {unsigned v=region==0?0xff4080ffu:region==1?0xffffce50u:region==2?0xffff3030u:0xffffce50u; // reload hand yellow (as before); two-hand grab: palm red, grab point purple, bright magenta in reach
                if(region==5)v=0xff00dfffu;if(region==6)v=0xff40ff60u;
                if(FormatFamily(g_format)==DXGI_FORMAT_R8G8B8A8_TYPELESS)v=(v&0xff00ff00u)|((v&0xff0000u)>>16)|((v&0xffu)<<16);
                pixels[y*448+x]=v;}
        }
        ID3D11DeviceContext* context=nullptr;device->GetImmediateContext(&context);context->UpdateSubresource(g_reloadMarkerUpload,0,nullptr,pixels,448*4,0);context->Release();g_reloadMarkerColor=color;
    }
    device->Release();return true;
}
template<size_t N> bool ReloadMarkerLayers(XrCompositionLayerQuad (&quads)[N],ID3D11Texture2D* frame)
{
    const bool grab=g_guideInsertOpen||GetTickCount64()-g_grabMarkersAt<150; // set every game frame while the gizmo applies
    if(!grab)for(int i=10;i<19;++i)g_reloadMarkerVisible[i]=false;
    if(!AnyGuideVisible()||!g_referenceValid||!g_viewsValid||!ReloadMarkerResources(frame,g_reloadMarkerState+(ReloadBoxFaceVisible()?2:0)+(g_reloadHandSphere?4:0)))return false;
    UINT index=0;XrSwapchainImageAcquireInfo acquire={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if(!XR_SUCCEEDED(p_xrAcquireSwapchainImage(g_reloadMarkerChain,&acquire,&index)))return false;
    XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;
    const bool copied=XR_SUCCEEDED(p_xrWaitSwapchainImage(g_reloadMarkerChain,&wait))&&index<g_reloadMarkerCount;
    if(copied)fc3stereo::CopyTexture(g_reloadMarkerImages[index],g_reloadMarkerUpload);
    XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};p_xrReleaseSwapchainImage(g_reloadMarkerChain,&release);if(!copied)return false;
    for(size_t i=0;i<N;++i)
    {
        quads[i]={XR_TYPE_COMPOSITION_LAYER_QUAD};quads[i].layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        quads[i].space=g_space;quads[i].eyeVisibility=XR_EYE_VISIBILITY_BOTH;quads[i].pose=g_reloadMarkerPose[i];
        const bool hand=i==1||i==8||i==9;float diameter=hand?g_reloadHandDiameter:g_reloadMarkerRadius*2;int region=hand?1:0;
        if(i==10||i>=14){diameter=g_grabPointDiameter;region=3;}else if(i>10){diameter=g_grabPalmDiameter;region=2;}
        quads[i].size=i>=2&&i<8?g_reloadBoxSize[i-2]:XrExtent2Df{diameter,diameter};
        if(i>=19){quads[i].size=g_doorHighlightSize363[i-19];region=g_doorHighlightHeld363?6:5;}
        quads[i].subImage.swapchain=g_reloadMarkerChain;quads[i].subImage.imageRect.offset={region*64,0};quads[i].subImage.imageRect.extent={64,64};
    }
    return true;
}

void ReloadMarkerSphere()
{
    g_reloadHandSphere=g_reloadMarkerVisible[1];
    g_reloadMarkerVisible[8]=g_reloadMarkerVisible[9]=g_reloadHandSphere;
    if(!g_reloadHandSphere)return;
    const float h=.70710678118f;
    const XrQuaternionf yaw={0,std::sin(g_referenceYaw*.5f),0,std::cos(g_referenceYaw*.5f)};
    g_reloadMarkerPose[1].orientation=yaw;
    g_reloadMarkerPose[8]=g_reloadMarkerPose[9]=g_reloadMarkerPose[1];
    g_reloadMarkerPose[8].orientation=testnotes::Multiply(yaw,{0,h,0,h});
    g_reloadMarkerPose[9].orientation=testnotes::Multiply(yaw,{h,0,0,h});
}
void ReloadMarkerPoints(const float* port,const float* shell,float radius,int contact,float handDiameter=.014f)
{
    for(int i=2;i<10;++i)g_reloadMarkerVisible[i]=false;g_reloadHandSphere=false;
    g_reloadHandDiameter=std::isfinite(handDiameter)?(std::max)(.01f,(std::min)(.20f,handDiameter)):.014f; // 20 cm max (the slider allowed it, this capped it at 10)
    g_reloadMarkerVisible[0]=port&&g_referenceValid&&std::isfinite(radius)&&radius>0;g_reloadMarkerVisible[1]=g_reloadMarkerVisible[0]&&shell;
    g_reloadMarkerRadius=radius/(std::max)(.1f,fc3stereo::WorldScale());g_reloadMarkerState=contact?1:0;
    const float cy=std::cos(-g_referenceYaw),sy=std::sin(-g_referenceYaw),scale=(std::max)(.1f,fc3stereo::WorldScale());
    for(int i=0;i<2;++i)if(g_reloadMarkerVisible[i])
    {
        const float* p=i?shell:port;const float x=p[0]/scale,y=p[1]/scale,z=-p[2]/scale;
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)){g_reloadMarkerVisible[i]=false;continue;}
        g_reloadMarkerPose[i].position={g_referencePosition.x+cy*x-sy*z,g_referencePosition.y+y,g_referencePosition.z+sy*x+cy*z};
        g_reloadMarkerPose[i].orientation=g_views[0].pose.orientation;
    }
}

// Grab gizmo: the point on the gun where the left hand takes the two-handed hold, and the left palm sphere. Body axes, game
// units (like ReloadMarkerPoints); diameters in metres. The two touch exactly when the grab is in reach.
void GrabMarkerPoints(const float* point,const float* palm,bool inReach,float pointDiameter,float palmDiameter)
{
    if(!point||!palm||!g_referenceValid)return;
    const float cy=std::cos(-g_referenceYaw),sy=std::sin(-g_referenceYaw),scale=(std::max)(.1f,fc3stereo::WorldScale());
    XrVector3f at[2]={};
    for(int k=0;k<2;++k){const float* p=k?palm:point;const float x=p[0]/scale,y=p[1]/scale,z=-p[2]/scale;if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return;
        at[k]={g_referencePosition.x+cy*x-sy*z,g_referencePosition.y+y,g_referencePosition.z+sy*x+cy*z};}
    const float h=.70710678118f;const XrQuaternionf yaw={0,std::sin(g_referenceYaw*.5f),0,std::cos(g_referenceYaw*.5f)};
    g_reloadMarkerPose[10]={g_views[0].pose.orientation,at[0]};
    g_reloadMarkerPose[11]={yaw,at[1]};g_reloadMarkerPose[12]={testnotes::Multiply(yaw,{0,h,0,h}),at[1]};g_reloadMarkerPose[13]={testnotes::Multiply(yaw,{h,0,0,h}),at[1]};
    g_grabPointDiameter=(std::max)(.01f,(std::min)(.5f,pointDiameter));g_grabPalmDiameter=(std::max)(.01f,(std::min)(.3f,palmDiameter));g_grabInReach=inReach;
    for(int i=10;i<14;++i)g_reloadMarkerVisible[i]=true;
    // Three nearest faces of a real yellow cube, with the existing saved diameter.
    XrQuaternionf cube=g_views[0].pose.orientation;
    const float norm=std::sqrt(cube.x*cube.x+cube.y*cube.y+cube.z*cube.z+cube.w*cube.w);
    if(!std::isfinite(norm)||norm<1e-6f)cube=yaw;
    else {cube.x/=norm;cube.y/=norm;cube.z/=norm;cube.w/=norm;}
    const XrQuaternionf face[6]={{0,0,0,1},{0,1,0,0},{0,h,0,h},{0,-h,0,h},{-h,0,0,h},{h,0,0,h}};
    const int axes[6]={2,2,0,0,1,1},slots[6]={10,14,15,16,17,18};
    XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
    for(int f=0;f<6;++f){XrVector3f local={};(&local.x)[axes[f]]=g_grabPointDiameter*.5f*(f%2?-1.f:1.f);const auto d=testnotes::Rotate(cube,local),atFace=testnotes::Add(at[0],d);int slot=slots[f];g_reloadMarkerPose[slot]={testnotes::Multiply(cube,face[f]),atFace};
      g_reloadMarkerVisible[slot]=d.x*(head.x-atFace.x)+d.y*(head.y-atFace.y)+d.z*(head.z-atFace.z)>0;}
    g_grabMarkersAt=GetTickCount64();
}

// Six transparent outlined faces form a spatial box; each pose follows weapon axes.
void ReloadMarkerBox(const float* m,const float* dims,int contact)
{
 for(int i=2;i<8;++i)g_reloadMarkerVisible[i]=false;
 if(!m||!dims||!g_referenceValid||!g_reloadMarkerVisible[0])return;
 for(int i=0;i<16;++i)if(!std::isfinite(m[i]))return;
 for(int i=0;i<3;++i)if(!std::isfinite(dims[i])||dims[i]<=0)return;
 const float scale=(std::max)(.1f,fc3stereo::WorldScale()),h=.70710678118f;XrPosef bodyPose={};if(!fc3xr::BodyBoxToReferencePose(m,scale,g_referenceYaw,g_referencePosition,bodyPose))return;const XrQuaternionf q=bodyPose.orientation;
 const XrQuaternionf face[6]={{0,0,0,1},{0,1,0,0},{0,h,0,h},{0,-h,0,h},{-h,0,0,h},{h,0,0,h}};
 const int axes[6]={2,2,0,0,1,1};
 const XrVector3f center=bodyPose.position;
 // Only the faces turned toward the head are drawn (the boxes appeared to rotate oddly on every weapon). All six see-through
 // outlines at once, composited over the gun, form an ambiguous wire cube that the eye flips (Necker cube): it looked as if
 // the box turned the wrong way. The visible faces of a solid box cannot flip.
 const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
 int shown=0;
 for(int i=0;i<6;++i){int axis=axes[i];XrVector3f local={};float* v=&local.x;v[axis]=dims[axis]/scale*.5f*(i%2?-1.f:1.f);auto delta=testnotes::Rotate(q,local);g_reloadMarkerPose[i+2]={testnotes::Multiply(q,face[i]),testnotes::Add(center,delta)};g_reloadBoxSize[i]={dims[axis==0?2:0]/scale,dims[axis==1?2:1]/scale};
  const XrVector3f faceCentre=testnotes::Add(center,delta);const float toHead[3]={head.x-faceCentre.x,head.y-faceCentre.y,head.z-faceCentre.z};
  const float facing=delta.x*toHead[0]+delta.y*toHead[1]+delta.z*toHead[2]; // delta = the face's outward direction
  g_reloadMarkerVisible[i+2]=!g_viewsValid||facing>0;if(g_reloadMarkerVisible[i+2])++shown;}
 if(!shown)for(int i=0;i<6;++i)g_reloadMarkerVisible[i+2]=true; // head inside the box: keep all
 g_reloadMarkerVisible[0]=false;g_reloadMarkerState=contact?1:0;
}
