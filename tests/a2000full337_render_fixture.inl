HRESULT STDMETHODCALLTYPE MirrorCreate337(ID3D11Device* d,const D3D11_BUFFER_DESC* desc,const D3D11_SUBRESOURCE_DATA* data,ID3D11Buffer** out){return d->CreateBuffer(desc,data,out);}
void TestA2000Full337(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>5e-4)return false;return true;};
 auto determinant=[](const Mat4& m){return m.m[0]*(m.m[5]*m.m[10]-m.m[6]*m.m[9])-m.m[1]*(m.m[4]*m.m[10]-m.m[6]*m.m[8])+m.m[2]*(m.m[4]*m.m[9]-m.m[5]*m.m[8]);};
 const auto create=g_oCreateBuffer;g_oCreateBuffer=MirrorCreate337;const auto guides=g_pistolGuides;
 ID3D11Buffer* oldVb=nullptr,*oldIb=nullptr;UINT oldStride=0,oldOffset=0,oldIbOffset=0;DXGI_FORMAT oldFormat={};context->IAGetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);context->IAGetIndexBuffer(&oldIb,&oldFormat,&oldIbOffset);
 auto* vb=Buffer(device,214976,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,45228,D3D11_BIND_INDEX_BUFFER);UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 g_weaponMechanicProfile=7;g_weaponMechanicFrame=++g_frame;g_pistolMenu.open=false;g_twoHand=false;g_weaponMotionGuard=false;g_inGameplay=g_motionGun=g_handRightValid=g_handLeftValid=true;
 fc3aim::SetNativeActionForTest128(0);fc3aim::SetVehicleStateForTest(0);fc3pad::SetGameplayControls(true);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 Mat4 mirror=HandIdentity();mirror.m[0]=-1;float source[88]={};WritePistolWorld(source,HandIdentity());float physical[2][16]={};for(int h=0;h<2;++h){physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
 auto draw=[&](UINT start,UINT count,const Mat4& native){float rows[88]={};WritePistolWorld(rows,native);cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);auto* output=BuildWeaponWorld(context,start,count);Check(output!=nullptr,"337 actual A2000 GPU world override available");float result[88]={};Read(device,context,output,result);BOOL tagged=FALSE;UINT n=sizeof(tagged);Check(SUCCEEDED(output->GetPrivateData(kA2000Mirror337,&n,&tagged)),"337 GPU output carries reflection winding tag");return std::make_pair(HullWorld(result),std::make_pair(output,tagged));};
 for(int owner:{0,1}){
  SetRenderHands303(owner,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_pistolReload={};auto& p=g_pistolReload;p.enabled=p.magazinePresent=p.chamberReady=true;g_frameGunValid=true;g_frameGunMove=HandIdentity();g_rifleAttachFrame=-100000;
  const auto frame=draw(0,8754,HandIdentity());const Mat4 root=p.rootWorld;
  Check(equal(frame.first,owner?Multiply(root,mirror):root),"337 actual uploaded A2000 FRAME mesh is fully reflected only for left owner");
  Check((determinant(frame.first)*determinant(root)<0)==(owner!=0)&&frame.second.second==(owner?TRUE:FALSE),"337 actual frame upload changes vertex handedness and winding tag,not just lever origin");
  const V3 vertex={.12,.23,.31};const auto actual=Apply(frame.first,vertex),expected=Apply(root,{owner?-.12:.12,.23,.31});Check(Length(Sub(actual,expected))<.001,"337 actual asymmetric weapon vertices appear on opposite side");
  for(auto material: {std::pair<UINT,UINT>{8754,2736},{11490,8184}}){auto f=draw(material.first,material.second,HandIdentity());Check(equal(f.first,frame.first),"337 every A2000 body material shares full mesh reflection");}
  p.parentValid=true;p.parentEpoch=g_armPoseEpoch;for(double pull:{0.,.025,.05}){
   p.rackHeld=true;p.slidePull=pull;Mat4 native=kA2000PartRest0;native.m[7]-=pull*kA2000ChargeTravel/.05;auto lever=draw(21066,528,kA2000PartRest0);
   Check(equal(lever.first,Multiply(p.rootWorld,Multiply(owner?mirror:HandIdentity(),native))),"337 actual uploaded charging mesh mirrors entire local geometry at every rail position");
  }
  p.rackHeld=false;p.slidePull=0;p.insertActive=false;p.dropActive=false;const auto magazine=draw(19674,1392,kA2000ClipRest);Check(equal(magazine.first,Multiply(p.rootWorld,Multiply(owner?mirror:HandIdentity(),kA2000ClipRest))),"337 seated magazine matches fully mirrored receiver");
  p.held=true;p.handFrame=g_frame;p.hand=HandIdentity();p.hand.m[3]=.7;p.hand.m[7]=.2;const auto carried=draw(19674,1392,kA2000ClipRest);Check(equal(carried.first,owner?Multiply(p.hand,mirror):p.hand),"337 carried magazine reflects at actual free hand instead of jumping across receiver");p.held=false;
  BeginPistolInsert();CameraSummary camera={};Mat4 vi={};Check(PistolGuideCamera(context,camera)&&Invert(HullView(camera),vi),"337 insertion camera available");
  Check(equal(Multiply(p.rootWorld,Multiply(owner?mirror:HandIdentity(),p.insertStart)),Multiply(vi,owner?Multiply(p.hand,mirror):p.hand)),"337 actual BeginPistolInsert starts at carried magazine without crossing gun");p.insertActive=false;
  for(auto geometry:{std::pair<UINT,UINT>{10560,2160},{67448,11280},{48520,7032}}){
   auto* av=Buffer(device,geometry.first,D3D11_BIND_VERTEX_BUFFER);auto* ai=Buffer(device,geometry.second,D3D11_BIND_INDEX_BUFFER);context->IASetVertexBuffers(0,1,&av,&stride,&offset);context->IASetIndexBuffer(ai,DXGI_FORMAT_R16_UINT,0);
   Mat4 attachment=HandIdentity();attachment.m[3]=.14;attachment.m[7]=.25;float rows[88]={};WritePistolWorld(rows,attachment);cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
   if(owner)Check(BuildWeaponPalette(context,0,1080)==nullptr,"337 attachment palette routes to whole-world reflection,including skinned holographic model");
   const auto optic=draw(0,1080,attachment);Check(equal(optic.first,Multiply(p.rootWorld,Multiply(owner?mirror:HandIdentity(),attachment))),"337 actual attachment GPU world follows full A2000 reflection");
   context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);av->Release();ai->Release();
  }
  ID3D11RasterizerState* before=nullptr;context->RSGetState(&before);ID3D11RasterizerState* saved=nullptr;bool changed=false;Check(BeginA2000Raster337(context,frame.second.first,saved,changed),"337 mirrored mesh raster scope succeeds");Check(changed==(owner!=0),"337 face winding changes only for genuinely reflected GPU buffer");
  if(owner){ID3D11RasterizerState* active=nullptr;context->RSGetState(&active);D3D11_RASTERIZER_DESC prior={},current={};if(before)before->GetDesc(&prior);else{prior.FillMode=D3D11_FILL_SOLID;prior.CullMode=D3D11_CULL_BACK;prior.DepthClipEnable=TRUE;}active->GetDesc(&current);Check(current.FrontCounterClockwise!=prior.FrontCounterClockwise&&current.CullMode==prior.CullMode,"337 mirrored face winding reverses while retaining original cull mode");SafeRelease(active);}
  EndA2000Raster337(context,saved,changed);ID3D11RasterizerState* restored=nullptr;context->RSGetState(&restored);Check(restored==before,"337 exact game rasterizer restored after both eyes");SafeRelease(restored);SafeRelease(before);
  printf("PASS:337 A2000 owner%d actual GPU FRAME/materials/lever/magazine reflection and rasterizer restoration\n",owner);
 }
 context->IASetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);context->IASetIndexBuffer(oldIb,oldFormat,oldIbOffset);SafeRelease(oldVb);SafeRelease(oldIb);vb->Release();ib->Release();g_oCreateBuffer=create;g_pistolGuides=guides;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;
}
