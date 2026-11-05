bool g_noteOptions=false;wchar_t g_optionStatus[512]={};
XrSwapchain g_noteChain=XR_NULL_HANDLE;
ID3D11Texture2D* g_noteImages[8]={};uint32_t g_noteImageCount=0;
ID3D11Texture2D* g_noteUpload=nullptr;
bool g_noteVisible=false,g_notePlaced=false,g_noteDragging=false,g_noteChord=false,g_noteHover=false,g_noteDirty=true,g_noteFailed=false;
XrPosef g_notePose={{0,0,0,1},{0,0,-.9f}},g_noteRelative={};
wchar_t g_noteStatus[512]=L"Preparing capture...",g_noteSteps[2048]=L"Hold LEFT grip + tap X: take one shell.\nKeep grip held; move shell into BLUE ring.\nOne insertion = loaded +1 / reserve -1.\nTake another shell after the reload finishes.\nAlignment: LEFT grip + B toggles.\nLeft stick X/Y; right stick up/down Z.\nX saves alignment. Same chord exits.\nQuit normally when done.";
ULONGLONG g_noteFileCheck=0;
void NotesPlace()
{
    if(g_notePlaced||!g_viewsValid)return;g_notePose.orientation=g_views[0].pose.orientation;
    const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
    g_notePose.position=testnotes::Add(head,testnotes::Rotate(g_notePose.orientation,{-.40f,-.08f,-.90f}));g_notePlaced=true;
}
void NotesControls(fc3pad::Controllers& c)
{
    g_noteVisible=false;g_noteDragging=false;g_noteChord=false;return; // disabled: never consume controller input
    if(fc3pad::MenuOpen()||g_noteOptions){g_noteDragging=false;NotesPlace();return;}
    const bool chord=c.active&&c.leftGrip>.7f&&c.y;
    if(chord&&!g_noteChord){g_noteVisible=!g_noteVisible;g_noteDragging=false;if(g_noteVisible)g_notePlaced=false;Log("[XR NOTES] %s",g_noteVisible?"shown":"hidden");}
    g_noteChord=chord;if(chord){c.leftGrip=0;c.y=false;return;}
    if(!g_noteVisible||!c.active){g_noteDragging=false;return;}
    NotesPlace();XrSpaceLocation location={XR_TYPE_SPACE_LOCATION};
    if(!g_notePlaced||g_handSpaces[1]==XR_NULL_HANDLE||!XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[1],g_space,g_displayTime,&location))||
       (location.locationFlags&(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))!=(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
    {g_noteDragging=false;return;}
    const bool hover=testnotes::Hit(location.pose,g_notePose);
    if(hover!=g_noteHover){g_noteHover=hover;g_noteDirty=true;}
    if(c.leftGrip>.7f)
    {
        if(!g_noteDragging&&hover){g_noteRelative=testnotes::Relative(location.pose,g_notePose);g_noteDragging=true;Log("[XR NOTES] panel grabbed");}
        if(g_noteDragging){g_notePose=testnotes::Follow(location.pose,g_noteRelative);c.leftGrip=0;}
    }
    else if(g_noteDragging){g_noteDragging=false;Log("[XR NOTES] panel parked");}
}
bool NotesResources(ID3D11Device* device)
{
    if(g_noteFailed)return false;if(g_noteUpload)return true;
    XrSwapchainCreateInfo info={XR_TYPE_SWAPCHAIN_CREATE_INFO};info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    info.format=g_format;info.sampleCount=1;info.width=testnotes::Width;info.height=testnotes::Height;info.faceCount=1;info.arraySize=1;info.mipCount=1;
    if(!Ok(p_xrCreateSwapchain(g_session,&info,&g_noteChain),"notes xrCreateSwapchain")){g_noteFailed=true;return false;}
    uint32_t count=0;p_xrEnumerateSwapchainImages(g_noteChain,0,&count,nullptr);
    if(count==0||count>8){p_xrDestroySwapchain(g_noteChain);g_noteChain=XR_NULL_HANDLE;g_noteFailed=true;return false;}
    XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};
    if(!XR_SUCCEEDED(p_xrEnumerateSwapchainImages(g_noteChain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images)))){g_noteFailed=true;return false;}
    for(UINT i=0;i<count;++i)g_noteImages[i]=images[i].texture;g_noteImageCount=count;
    D3D11_TEXTURE2D_DESC texture={};texture.Width=testnotes::Width;texture.Height=testnotes::Height;texture.MipLevels=1;texture.ArraySize=1;texture.Format=g_format;texture.SampleDesc.Count=1;texture.Usage=D3D11_USAGE_DEFAULT;
    if(FAILED(device->CreateTexture2D(&texture,nullptr,&g_noteUpload))){g_noteFailed=true;return false;}
    g_noteDirty=true;Log("[XR NOTES] native stereo quad ready, %dx%d",testnotes::Width,testnotes::Height);return true;
}
void NotesReadFile()
{
    const auto now=GetTickCount64();if(now-g_noteFileCheck<1000)return;g_noteFileCheck=now;
    wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_test_notes.txt",fc3::DataFolder());
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return;
    char bytes[4096]={};DWORD read=0;const bool ok=ReadFile(file,bytes,sizeof(bytes)-1,&read,nullptr)!=FALSE;CloseHandle(file);
    if(!ok||!read)return;wchar_t text[2048]={};const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes,read,text,2047);if(!count)return;text[count]=0;
    if(wcscmp(text,g_noteSteps)){wcscpy_s(g_noteSteps,text);g_noteDirty=true;}
}
bool SelectorPreparing293();
bool NotesLayer(XrCompositionLayerQuad& quad,ID3D11Texture2D* frame)
{
    if(fc3pad::MenuOpen())return OptionsMenuLayer(quad);
    // Only the automatic weapon preparation status uses this layer. The old
    // diagnostic panel and its controller shortcuts remain disabled.
    if(!SelectorPreparing293()){g_notePlaced=false;return false;}NotesPlace();if(!g_notePlaced)return false;
    ID3D11Device* device=nullptr;frame->GetDevice(&device);const bool resources=NotesResources(device);device->Release();if(!resources)return false;
    if(g_noteDirty)
    {
        std::vector<unsigned> pixels;if(!testnotes::Pixels(L"Preparing your weapons...",L"Release A and grip.\nYour four weapons will appear in front of you.\nThen reach for a handle and grip to equip.\nPress A to cancel.",false,pixels,L"WEAPONS"))return false;
        if(FormatFamily(g_format)==DXGI_FORMAT_R8G8B8A8_TYPELESS)
            for(auto& color:pixels)color=(color&0xff00ff00u)|((color&0x00ff0000u)>>16)|((color&0x000000ffu)<<16);
        frame->GetDevice(&device);ID3D11DeviceContext* context=nullptr;device->GetImmediateContext(&context);device->Release();
        context->UpdateSubresource(g_noteUpload,0,nullptr,pixels.data(),testnotes::Width*4,0);context->Release();g_noteDirty=false;
    }
    UINT index=0;XrSwapchainImageAcquireInfo acquire={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};if(!XR_SUCCEEDED(p_xrAcquireSwapchainImage(g_noteChain,&acquire,&index)))return false;
    XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;
    const bool copied=XR_SUCCEEDED(p_xrWaitSwapchainImage(g_noteChain,&wait))&&index<g_noteImageCount;
    if(copied)fc3stereo::CopyTexture(g_noteImages[index],g_noteUpload);
    XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};p_xrReleaseSwapchainImage(g_noteChain,&release);if(!copied)return false;
    quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=g_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;quad.pose=g_notePose;
    quad.size={testnotes::PanelWidth,testnotes::PanelHeight};quad.subImage.swapchain=g_noteChain;quad.subImage.imageRect.extent={testnotes::Width,testnotes::Height};return true;
}
