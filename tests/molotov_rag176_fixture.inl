void TestMolotovRag176(ID3D11Device* device,ID3D11DeviceContext* context)
{
 float savedHand[16];memcpy(savedHand,g_handBody[0],sizeof(savedHand));
 Mat4 to={},from={};float unused[576]={};CameraSummary camera={};
 Check(DrawSpace(context,unused,to,from)&&PistolGuideCamera(context,camera),"176 actual rag draw transforms available");
 Mat4 cameraToBody={},cameraToWorld={};Check(Invert(ArmBodyToCamera(),cameraToBody)&&Invert(HullView(camera),cameraToWorld),"176 rag body/world transforms invertible");
 const auto savedCamera=g_frameCamera;const bool savedValid=g_frameCameraValid;g_frameCamera=camera;g_frameCameraValid=true;
 const V3 firePoint={.012176885269582272,-.03637806296348572,.17822809517383575};
 const auto savedRag177=g_molotovRag177;g_molotovRag177={};
 const auto& key=kThrowMolotovClipB[kThrowMolotovBIdx_rest];
 const double wrongBoneDistance=Length(Sub(Apply(ThrowMat144(key.part[1]),firePoint),Apply(ThrowMat144(key.part[2]),firePoint)));
 Check(wrongBoneDistance>.01,"176 real native rest bottle and cloth anchors differ materially");
 std::printf("176 wrong-bone rest displacement %.3f mm\n",wrongBoneDistance*1000);
 for(int turn=0;turn<3;++turn){
  memcpy(g_handBody[0],savedHand,sizeof(savedHand));const double angle=turn*.9;
  g_handBody[0][0]=g_handBody[0][5]=float(cos(angle));g_handBody[0][1]=float(-sin(angle));g_handBody[0][4]=float(sin(angle));g_handBody[0][3]+=.13f*turn;
  auto* palette=ThrowablePalette144(context,0,2799);Check(palette!=nullptr,"177 actual TWO-SLOT cloth GPU palette uploaded");
  float actual[576]={};Read(device,context,palette,actual);
  const Mat4 renderedClothCamera=Multiply(to,BoneMatrix(actual,0)),renderedAnchorCamera=Multiply(to,BoneMatrix(actual,1));
  const V3 actualClothCamera=Add(Scale(Apply(renderedClothCamera,firePoint),254./255.),Scale(Apply(renderedAnchorCamera,firePoint),1./255.));
  const V3 expectedBody=Apply(cameraToBody,actualClothCamera);
  const V3 expectedWorld=Apply(cameraToWorld,actualClothCamera);
  V3 wick={},nearest={};double distance=0;Mat4 flame={};
  Check(MolotovWickBody158(wick)&&Length(Sub(wick,expectedBody))<1e-5,"177 contact follows actual weighted hanging CLOTH vertex under rotation");
  Check(RagContact159(expectedBody,nearest,distance)&&distance<1e-5,"177 ignition capsule includes actual weighted hanging cloth tip");
  Check(MolotovFlameWorld176(camera,flame)&&Length(Sub(Apply(flame,{0,0,0}),expectedWorld))<1e-5,"177 native emitter origin matches actual weighted CLOTH tip");
  const Mat4 worldCloth=Multiply(cameraToWorld,renderedClothCamera);
  const V3 emitterY=Sub(Apply(flame,{0,1,0}),Apply(flame,{0,0,0}));
  const V3 clothZ=Sub(Apply(worldCloth,{0,0,1}),Apply(worldCloth,{0,0,0}));
  Check(Length(Sub(emitterY,clothZ))<1e-5,"178 authored FX_FIRE +90X preserved: emitter Y follows cloth Z, with no duplicate node translation");

 }
 g_molotovRag177.contact[0]=.03f;g_molotovRag177.flame[2]=-.02f;
 Mat4 adjustedFrame={};Check(ThrowableFrame144(camera,adjustedFrame),"177 edited rag frame");
 V3 editedWick={};Check(MolotovWickBody158(editedWick)&&Length(Sub(editedWick,Apply(cameraToBody,MolotovRagPoint177(adjustedFrame,false))))<1e-5,"177 independent trigger offset reaches production contact");
 Mat4 editedFlame={};Check(MolotovFlameWorld176(camera,editedFlame)&&Length(Sub(Apply(editedFlame,{0,0,0}),Apply(cameraToWorld,MolotovRagPoint177(adjustedFrame,true))))<1e-5,"177 independent native flame offset reaches publication");
 g_molotovRag177=savedRag177;memcpy(g_handBody[0],savedHand,sizeof(savedHand));g_frameCamera=savedCamera;g_frameCameraValid=savedValid;
}
