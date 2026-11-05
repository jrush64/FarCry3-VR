void TestA2000Mirror336(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
 const auto guides=g_pistolGuides;const auto oldProfile=g_weaponMechanicProfile;Mat4 mirror=HandIdentity();mirror.m[0]=-1;
 auto reflect=[&](const Mat4& m){return Multiply(mirror,Multiply(m,mirror));};
 g_weaponMechanicProfile=7;g_weaponMechanicFrame=g_frame;g_pistolReload={};auto& p=g_pistolReload;p.enabled=true;p.magazinePresent=true;p.parentValid=false;g_twoHand=false;
 const Mat4 root=HandIdentity();g_pistolGuides.box=true;g_pistolGuides.charge[0]=.2f;g_pistolGuides.charge[1]=.3f;g_pistolGuides.charge[2]=.1f;g_pistolGuides.handDiameter=.02f;
 for(int i=0;i<3;++i){g_pistolGuides.size[2][i]=.04f;g_pistolGuides.angles[2][i]=float((i+1)*13);}
 SetRenderHands303(0,false,g_armPoseEpoch);const Mat4 originalBox=PistolGuideBox(root,3),originalPort=PistolGuideBox(root,0);const V3 originalGuide=PistolRackGuide();
 Check(!A2000LeftCharge336()&&equal(A2000ChargeTransform336(kA2000PartRest0),kA2000PartRest0),"336 right A2000 lever pose is byte-value unchanged");
 SetRenderHands303(1,false,g_armPoseEpoch);
 Check(A2000LeftCharge336()&&equal(PistolGuideBox(root,3),reflect(originalBox)),"336 A2000 charge contact centre and rotated box mirror together");
 Check(equal(PistolGuideBox(root,0),originalPort),"336 A2000 magazine insertion box stays unchanged");
 const auto guide=PistolRackGuide();Check(fabs(guide.x+originalGuide.x)<1e-6&&guide.y==originalGuide.y&&guide.z==originalGuide.z,"336 charging guide uses opposite side but original stroke axis");
 Check(PistolGuideContact(root,3,guide)&&!PistolGuideContact(root,3,originalGuide),"336 only mirrored A2000 charging box acquires left-owned reload");
 // Actual production rigid lever writer with native buffer classification,
 // across rest/partial/full stroke and spring return. Body and accessories untouched.
 ID3D11Buffer* oldVb=nullptr,*oldIb=nullptr;UINT oldStride=0,oldOffset=0,oldIbOffset=0;DXGI_FORMAT oldFormat={};context->IAGetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);context->IAGetIndexBuffer(&oldIb,&oldFormat,&oldIbOffset);
 auto* vb=Buffer(device,214976,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,45228,D3D11_BIND_INDEX_BUFFER);UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary camera={};Check(PistolGuideCamera(context,camera),"336 current camera available");Mat4 vi={};Check(Invert(HullView(camera),vi),"336 current camera invertible");
 for(int owner:{0,1}){SetRenderHands303(owner,false,g_armPoseEpoch);for(double pull:{0.,.025,.05})for(int phase:{0,1,2}){
  p.rootBody=root;p.rootNative=root;p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootContext=context;p.parentValid=true;p.parentEpoch=g_armPoseEpoch;p.controllerParent=HandIdentity();p.pending=p.cycle=p.held=p.removalHeld=false;p.rackHeld=phase==1;p.rackReturning=phase==2;p.slidePull=phase?pull:0;g_akParts[0]={};
  const Mat4 body=AkParentBody();Mat4 expected=kA2000PartRest0;if(phase)expected.m[7]=RifleChargeRestY()-pull*kA2000ChargeTravel/.05;if(owner)expected=reflect(expected);
  float out[88]={};WritePistolWorld(out,HandIdentity());AkFinalWorld(context,21066,528,kA2000PartRest0,out);
  Check(equal(HullWorld(out),Multiply(vi,Multiply(ArmBodyToCamera(),Multiply(body,expected)))),"336 production A2000 lever writer mirrors only left-owned part across full rail and return");
  Check(equal(A2000ChargeTransform336(kA2000PartRest0),owner?reflect(kA2000PartRest0):kA2000PartRest0),"336 no negative-determinant weapon-body reflection introduced");
 }}
 context->IASetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);context->IASetIndexBuffer(oldIb,oldFormat,oldIbOffset);SafeRelease(oldVb);SafeRelease(oldIb);vb->Release();ib->Release();
 for(int profile:{8,9,20,28,39,42}){g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;SetRenderHands303(1,false,g_armPoseEpoch);Check(!A2000LeftCharge336()&&equal(A2000ChargeTransform336(kA2000PartRest0),kA2000PartRest0),"336 other weapons cannot acquire A2000 side mirror");}
 g_pistolGuides=guides;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;p={};p.enabled=p.magazinePresent=p.chamberReady=true;
 puts("PASS:336 A2000-only opposite-side lever,rotated contact box,unchanged insertion/right owner,production rigid rail through stroke and return;offline only");
}
