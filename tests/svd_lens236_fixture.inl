void TestSvdLens236(ID3D11Device* device,ID3D11DeviceContext* context){
 const auto profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame,frame=g_frame;
 const auto motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand;
 const auto reload=g_reload;const auto bolt=g_r700;const auto menu=g_pistolMenu;const auto machete=g_machete;
 const auto neutral=g_controllerNeutral124;const auto attachment=g_freeRightFromController;const UINT epoch=g_freeRightEpoch;
 const auto create=g_oCreateBuffer;g_oCreateBuffer=PistolFixtureCreate;
 const auto controls=fc3pad::Latest();auto* ctx=g_ctx;auto* dev=g_dev;float hands[2][16];memcpy(hands,g_handBody,sizeof(hands));
 g_ctx=context;g_dev=device;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=g_twoHand=false;
 g_reload={};g_r700={};g_machete.held=false;g_pistolMenu.open=false;fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 g_weaponMechanicProfile=35;g_weaponMechanicFrame=++g_frame;const auto id=HandIdentity();
 for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=float(id.m[i]);
 auto* vb=Buffer(device,268464,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,59526,D3D11_BIND_INDEX_BUFFER);auto* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 CameraSummary cam={};cam.mainView=1;cam.right[0]=cam.up[1]=cam.forward[2]=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
 context->ClearState();context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);context->VSSetConstantBuffers(1,1,&camera);
 const UINT ranges[6][2]={{0,6},{6,432},{438,432},{1488,288},{1776,192},{2946,87}};
 for(int prefix=0;prefix<2;++prefix){
  const std::string hlsl=std::string(prefix?"float4 AlphaValues;":"")+"row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;float4 main(uint i:SV_VertexID):SV_Position{return float4(mul(_WorldMatrix,float4(i,0,0,1))+_WorldPositionFractions"+(prefix?"+AlphaValues.xyz":"")+",1);}";
  auto* code=HullCompile(hlsl.c_str(),"vs_5_0");ID3D11VertexShader* vs=nullptr;device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs);vs->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();context->VSSetShader(vs,nullptr,0);
  for(UINT bytes:{128u,336u,544u}){auto* cb=Buffer(device,bytes,D3D11_BIND_CONSTANT_BUFFER);context->VSSetConstantBuffers(0,1,&cb);
   for(int pose=0;pose<3;++pose){g_handBody[0][3]=.2f+pose*.1f;g_handBody[0][7]=-.2f;Mat4 root={};Check(ControllerMagazineRoot124(root),"236 SVD fixed receiver root available before lens/arm draw");const Mat4 expected=Multiply(ArmBodyToCamera(),root);
    float source[136];for(int i=0;i<136;++i)source[i]=.001f*i;Mat4 native=id;native.m[3]=7+pose;native.m[7]=-8;WritePistolWorld(source+prefix*4,native);HookUpdateSubresource(context,cb,0,nullptr,source,0,0);
    for(int layer=0;layer<6;++layer){UINT stride=layer<3?20:32,offset=layer<3?0:10160;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);DrawCall call={};call.b=ranges[layer][0];call.a=ranges[layer][1];
     auto* out=BuildSvdLens236(context,call);Check(out!=nullptr,"236 all six original SVD lens/illumination draws receive fixed root across globals layouts");float result[136]={},original[136]={};Read(device,context,out,result);Read(device,context,cb,original);Compare(source,original,bytes/4,"236 SVD lens preserves original GPU globals");const auto actual=HullWorld(result+prefix*4);for(int i=0;i<16;++i)Check(fabs(actual.m[i]-expected.m[i])<.0001,"236 uploaded SVD glass matches receiver despite animated native world");
     for(UINT i=0;i<bytes/4;++i)if(i<UINT(prefix*4)||i>=UINT(prefix*4+15))Check(result[i]==source[i],"236 lens material/prefix and unrelated globals unchanged");
     ++call.a;Check(!BuildSvdLens236(context,call),"236 coincident unrelated SVD geometry excluded");--call.a;g_weaponMechanicProfile=8;Check(!BuildSvdLens236(context,call),"236 another weapon cannot inherit SVD lens placement");g_weaponMechanicProfile=35;
    }
   }SafeRelease(cb);
  }SafeRelease(vs);
 }
 context->ClearState();SafeRelease(vb);SafeRelease(ib);SafeRelease(camera);ReleaseSvdLens236();
 g_oCreateBuffer=create;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_frame=frame;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;
 g_reload=reload;g_r700=bolt;g_pistolMenu=menu;g_machete=machete;g_controllerNeutral124=neutral;g_freeRightFromController=attachment;g_freeRightEpoch=epoch;g_ctx=ctx;g_dev=dev;memcpy(g_handBody,hands,sizeof(hands));fc3pad::Publish(controls);
 puts("PASS:236 six SVD glass/illumination layers match fixed receiver in actual GPU uploads, 128/336/544-byte globals, reflected prefixes, moving controller, immutable source and exact geometry/profile guards");
}
