
ID3D11Texture2D* g_optionsTexture=nullptr;
struct OptionsCrop {int x=0,y=0,width=0,height=0;}g_optionsCrop;
XrSwapchain g_optionsChain=XR_NULL_HANDLE;ID3D11Texture2D* g_optionsImages[8]={};UINT g_optionsCount=0,g_optionsWidth=0,g_optionsHeight=0;
bool OptionsMenuLayer(XrCompositionLayerQuad& quad)
{
 if(!g_optionsTexture||!g_viewsValid)return false;
 D3D11_TEXTURE2D_DESC td={};g_optionsTexture->GetDesc(&td);
 if(!g_optionsChain||g_optionsWidth!=td.Width||g_optionsHeight!=td.Height)
 {
  if(g_optionsChain)p_xrDestroySwapchain(g_optionsChain);g_optionsChain=XR_NULL_HANDLE;g_optionsCount=0;
  XrSwapchainCreateInfo info={XR_TYPE_SWAPCHAIN_CREATE_INFO};info.format=DXGI_FORMAT_R8G8B8A8_UNORM;
  info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;info.width=td.Width;info.height=td.Height;info.sampleCount=info.faceCount=info.arraySize=info.mipCount=1;
  if(!XR_SUCCEEDED(p_xrCreateSwapchain(g_session,&info,&g_optionsChain)))return false;
  UINT count=0;p_xrEnumerateSwapchainImages(g_optionsChain,0,&count,nullptr);
  if(!count||count>8){p_xrDestroySwapchain(g_optionsChain);g_optionsChain=XR_NULL_HANDLE;return false;}
  XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};
  if(!XR_SUCCEEDED(p_xrEnumerateSwapchainImages(g_optionsChain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images)))){p_xrDestroySwapchain(g_optionsChain);g_optionsChain=XR_NULL_HANDLE;return false;}
  for(UINT i=0;i<count;++i)g_optionsImages[i]=images[i].texture;g_optionsCount=count;g_optionsWidth=td.Width;g_optionsHeight=td.Height;
 }
 UINT index=0;XrSwapchainImageAcquireInfo acquire={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
 if(!XR_SUCCEEDED(p_xrAcquireSwapchainImage(g_optionsChain,&acquire,&index)))return false;
 XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;
 const auto waitStarted56=GetTickCount64();const auto waited56=p_xrWaitSwapchainImage(g_optionsChain,&wait);const auto waitedMs56=GetTickCount64()-waitStarted56;static LONG reports56=0;if(waitedMs56>=40&&reports56++<64)Log("[INSERT PERF56] options swapchain wait blocked %llu ms; result %d",waitedMs56,int(waited56));
 bool ok=XR_SUCCEEDED(waited56)&&index<g_optionsCount;
 if(ok)fc3stereo::CopyTexture(g_optionsImages[index],g_optionsTexture);
 XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};p_xrReleaseSwapchainImage(g_optionsChain,&release);if(!ok)return false;
 quad={XR_TYPE_COMPOSITION_LAYER_QUAD};quad.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;quad.space=g_space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;
 int width=g_optionsCrop.width>0?g_optionsCrop.width:td.Width,height=g_optionsCrop.height>0?g_optionsCrop.height:td.Height;
 // Keep the same virtual desktop plane: dragging moves the quad, shrinking
 // the window shrinks the quad instead of magnifying the cropped texture.
 const float metresPerPixel=fc3ui::OptionsMetres299(float(td.Width),float(td.Height));
 const float x=(g_optionsCrop.x+width*.5f-td.Width*.5f)*metresPerPixel;
 const float y=-(g_optionsCrop.y+height*.5f-td.Height*.5f)*metresPerPixel;
 quad.pose=g_views[0].pose;quad.pose.position=testnotes::Add(quad.pose.position,testnotes::Rotate(quad.pose.orientation,{x,y,-1.15f}));
 quad.size={width*metresPerPixel,height*metresPerPixel};quad.subImage.swapchain=g_optionsChain;
 quad.subImage.imageRect.offset={g_optionsCrop.x,g_optionsCrop.y};quad.subImage.imageRect.extent={width,height};return true;
}
