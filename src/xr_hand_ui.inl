#include "hand_ui_format.h"
ID3D11Texture2D* g_nativeHandUi=nullptr;
bool g_handUiCopyReady=false;
fc3handui::Settings g_nativeHandSettings;XrPosef g_nativeHandPose={};bool g_nativeHandValid=false,g_nativeHandWheel=false;
XrSwapchain g_handUiChain=XR_NULL_HANDLE;ID3D11Texture2D* g_handUiImages[8]={};UINT g_handUiCount=0,g_handUiWidth=0,g_handUiHeight=0;DXGI_FORMAT g_handUiSourceFormat=DXGI_FORMAT_UNKNOWN;
void HandUiXrFailure(const char* stage,XrResult result){static unsigned failures=0;if(failures++<12)Log("[HANDUI] XR failure stage=%s result=%ld sourceFormat=%u mainXRFormat=%u",stage,long(result),g_handUiSourceFormat,g_format);}
XrQuaternionf LevelHandUi345(XrQuaternionf panel,XrQuaternionf view){
 const auto normal=testnotes::Rotate(panel,{0,0,1}),up=testnotes::Rotate(panel,{0,1,0});
 // Prefer gravity for upright text. Near a horizontal watch face, project
 // head-up instead because gravity has no stable in-plane direction there.
 const XrVector3f eyeUp=std::fabs(normal.y)<.97f?XrVector3f{0,1,0}:testnotes::Rotate(view,{0,1,0});
 const float dot=eyeUp.x*normal.x+eyeUp.y*normal.y+eyeUp.z*normal.z;XrVector3f projected={eyeUp.x-dot*normal.x,eyeUp.y-dot*normal.y,eyeUp.z-dot*normal.z};const float length=std::sqrt(projected.x*projected.x+projected.y*projected.y+projected.z*projected.z);
 if(!std::isfinite(length)||length<.2f)return panel;projected={projected.x/length,projected.y/length,projected.z/length};
 const XrVector3f cross={up.y*projected.z-up.z*projected.y,up.z*projected.x-up.x*projected.z,up.x*projected.y-up.y*projected.x};const float angle=std::atan2(normal.x*cross.x+normal.y*cross.y+normal.z*cross.z,up.x*projected.x+up.y*projected.y+up.z*projected.z)*.5f;
 return testnotes::Multiply({normal.x*std::sin(angle),normal.y*std::sin(angle),normal.z*std::sin(angle),std::cos(angle)},panel);
}
XrPosef HandUiPanelPose(int i)
{
 const auto& p=g_nativeHandSettings.panels[i];XrPosef pose=g_nativeHandPose;
 pose.position=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,{p.x,p.y,-p.z}));
 const float d=.00872664626f;XrQuaternionf pitch={std::sin(p.pitch*d),0,0,std::cos(p.pitch*d)},yaw={0,std::sin(p.yaw*d),0,std::cos(p.yaw*d)},roll={0,0,std::sin(p.roll*d),std::cos(p.roll*d)};pose.orientation=testnotes::Multiply(pose.orientation,testnotes::Multiply(yaw,testnotes::Multiply(pitch,roll)));if(i<2&&g_nativeHandSettings.levelText345&&g_viewsValid)pose.orientation=LevelHandUi345(pose.orientation,g_views[0].pose.orientation);return pose;
}
bool HandUiLayers(XrCompositionLayerQuad (&quads)[3])
{
 static bool glanced285[2]={};
 g_handUiCopyReady=false;
 if(!g_nativeHandUi||!g_nativeHandValid||!g_nativeHandSettings.enabled||!g_referenceValid||!g_viewsValid)return false;
 D3D11_TEXTURE2D_DESC td={};g_nativeHandUi->GetDesc(&td);
 if(!g_handUiChain||td.Width!=g_handUiWidth||td.Height!=g_handUiHeight||td.Format!=g_handUiSourceFormat){if(g_handUiChain)p_xrDestroySwapchain(g_handUiChain);g_handUiChain=XR_NULL_HANDLE;g_handUiCount=0;
  XrSwapchainCreateInfo info={XR_TYPE_SWAPCHAIN_CREATE_INFO};int64_t formats[64]={};uint32_t formatCount=0;XrResult enumResult=p_xrEnumerateSwapchainFormats(g_session,0,&formatCount,nullptr);if(!XR_SUCCEEDED(enumResult)||formatCount>64){HandUiXrFailure("enumerate formats",enumResult);return false;}enumResult=p_xrEnumerateSwapchainFormats(g_session,formatCount,&formatCount,formats);if(!XR_SUCCEEDED(enumResult)){HandUiXrFailure("read formats",enumResult);return false;}info.format=fc3handui::ChooseFormat(td.Format,formats,formatCount);g_handUiSourceFormat=td.Format;if(info.format==DXGI_FORMAT_UNKNOWN){HandUiXrFailure("no same-family format",XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED);return false;}info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;info.width=td.Width;info.height=td.Height;info.sampleCount=info.faceCount=info.arraySize=info.mipCount=1;
  const auto creation=p_xrCreateSwapchain(g_session,&info,&g_handUiChain);if(!XR_SUCCEEDED(creation)){HandUiXrFailure("create swapchain",creation);return false;}Log("[HANDUI] XR swapchain ready sourceFormat=%u offeredChosen=%lld mainXRFormat=%u size=%ux%u%s",td.Format,info.format,g_format,td.Width,td.Height,(info.format==DXGI_FORMAT_R8G8B8A8_UNORM||info.format==DXGI_FORMAT_B8G8R8A8_UNORM)?" LINEAR FALLBACK":" SRGB");UINT count=0;p_xrEnumerateSwapchainImages(g_handUiChain,0,&count,nullptr);if(!count||count>8){HandUiXrFailure("image count",XR_ERROR_RUNTIME_FAILURE);return false;}XrSwapchainImageD3D11KHR images[8];for(UINT i=0;i<count;++i)images[i]={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};const auto imageResult=p_xrEnumerateSwapchainImages(g_handUiChain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images));if(!XR_SUCCEEDED(imageResult)){HandUiXrFailure("enumerate images",imageResult);return false;}for(UINT i=0;i<count;++i)g_handUiImages[i]=images[i].texture;g_handUiCount=count;g_handUiWidth=td.Width;g_handUiHeight=td.Height;
 }
 UINT index=0;XrSwapchainImageAcquireInfo acquire={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};const auto acquired=p_xrAcquireSwapchainImage(g_handUiChain,&acquire,&index);if(!XR_SUCCEEDED(acquired)){HandUiXrFailure("acquire image",acquired);return false;}XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;const auto waited=p_xrWaitSwapchainImage(g_handUiChain,&wait);bool ok=XR_SUCCEEDED(waited)&&index<g_handUiCount;if(ok)fc3stereo::CopyTexture(g_handUiImages[index],g_nativeHandUi);XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};const auto released=p_xrReleaseSwapchainImage(g_handUiChain,&release);if(!ok){HandUiXrFailure("wait/copy image",waited);return false;}if(!XR_SUCCEEDED(released)){HandUiXrFailure("release image",released);return false;}
 const bool visible[3]={g_nativeHandSettings.health,g_nativeHandSettings.ammo,g_nativeHandSettings.wheel&&g_nativeHandWheel};
 for(int i=0;i<3;++i){quads[i]={XR_TYPE_COMPOSITION_LAYER_QUAD};const auto& p=g_nativeHandSettings.panels[i];if(!visible[i]||!fc3handui::Valid(p))continue;
  auto& q=quads[i];q.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;q.space=g_space;q.eyeVisibility=XR_EYE_VISIBILITY_BOTH;q.pose=g_nativeHandPose;
  // User offsets use the drawn wrist's own axes in metres (z forward).
  q.pose=HandUiPanelPose(i);
  if(i<2&&g_nativeHandSettings.glanceOnly285&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()){
   const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
   const auto forward=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1}),normal=testnotes::Rotate(q.pose.orientation,{0,0,1});
   glanced285[i]=fc3handui::Glance285(&head.x,&forward.x,&q.pose.position.x,&normal.x,glanced285[i]);
   if(!glanced285[i])continue;
  }
  const int x=int(p.left*td.Width),y=int(p.top*td.Height),w=int(p.right*td.Width)-x,h=int(p.bottom*td.Height)-y;q.subImage.swapchain=g_handUiChain;q.subImage.imageRect.offset={x,y};q.subImage.imageRect.extent={w,h};q.size={p.width,p.width*float(h)/float(w)};
 }
 g_handUiCopyReady=true;return true;
}
