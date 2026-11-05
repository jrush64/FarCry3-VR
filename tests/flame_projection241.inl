void TestFlameProjection241(ID3D11Device* device,ID3D11DeviceContext* context){
 context->ClearState();const auto createVs=g_oCreateVertexShader;
 g_oCreateVertexShader=reinterpret_cast<CreateVertexShaderFn>((*reinterpret_cast<void***>(device))[12]);
 auto* code=HullCompile("float4 main(float3 p:POSITION):SV_Position{return float4(p,1);}","vs_5_0");
 ID3D11VertexShader* vs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"241 native NDC shader");
 const unsigned long long hash=0x8CBC4FD2A59FD8A3ull;vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);vs->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());
 D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};ID3D11InputLayout* layout=nullptr;Check(SUCCEEDED(device->CreateInputLayout(&element,1,code->GetBufferPointer(),code->GetBufferSize(),&layout)),"241 NDC layout");SafeRelease(code);
 D3D11_BUFFER_DESC desc={};desc.ByteWidth=147456;desc.Usage=D3D11_USAGE_DYNAMIC;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;ID3D11Buffer* vb=nullptr;Check(SUCCEEDED(device->CreateBuffer(&desc,nullptr,&vb)),"241 exact particle pool");UINT stride=36,offset=0;
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->VSSetShader(vs,nullptr,0);DrawCall call={};call.kind=1;call.a=3;g_weaponMechanicFrame=g_frame;
 Check(FlamerProjectedDraw241(context,call),"241 actual flamer draw selects depth-aware projection without shot callback or stale stream snapshot");
 const unsigned long long wrong=1;vs->SetPrivateData(kShaderHashGuid,sizeof(wrong),&wrong);Check(!FlamerProjectedDraw241(context,call),"241 unrelated shader retains original UI/world classification");vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
 ID3D11Buffer* originalVb=nullptr;UINT priorStride=0,priorOffset=0;Check(!BeginMuzzleFlashDraw111(context,call,0,originalVb,priorStride,priorOffset),"241 old flame stream mover cannot double-transport native births");
 ID3D11VertexShader* original=nullptr;Check(BeginFlamerProjection241(context,original)&&original==vs,"241 actual draw binds cached corrected VS");
 auto* gsCode=HullCompile("struct O{float4 p:SV_Position;};[maxvertexcount(1)]void main(point O i[1],inout PointStream<O> o){o.Append(i[0]);}","gs_5_0");D3D11_SO_DECLARATION_ENTRY entry={0,"SV_Position",0,0,4,0};UINT soStride=16;ID3D11GeometryShader* gs=nullptr;
 Check(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(gsCode->GetBufferPointer(),gsCode->GetBufferSize(),&entry,1,&soStride,1,D3D11_SO_NO_RASTERIZED_STREAM,nullptr,&gs)),"241 actual corrected vertex output readback");SafeRelease(gsCode);
 auto* output=Buffer(device,48,D3D11_BIND_STREAM_OUTPUT);auto* cb=Buffer(device,dxbc::kEyeRows*16,D3D11_BIND_CONSTANT_BUFFER);
 context->IASetInputLayout(layout);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);context->GSSetShader(gs,nullptr,0);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&cb);
 GameProjection game={1.3f,1.7f,1.0001f,-.01f};const Mat4 proj=ProjectionMatrix(game),unproj=ProjectionInverse(game);
 for(double head:{-.6,0.,.7})for(int eye=0;eye<2;++eye){
  Mat4 nativeView=TurnAndMove({0,0,1},{std::sin(head),0,std::cos(head)},{},{.04*head,0,0});Mat4 inverse={};Check(Invert(nativeView,inverse),"241 native view invert");
  Mat4 physicalEye=nativeView;physicalEye.m[3]+=(eye?-.032:.032);const Mat4 transform=Multiply(proj,Multiply(physicalEye,Multiply(inverse,unproj)));
  float rows[dxbc::kEyeRows*4]={};for(int i=0;i<16;++i)rows[i]=float(transform.m[i]-(i%5==0?1.:0.));
  rows[16]=.8f;rows[17]=-.7f;rows[20]=-.5f;rows[21]=-.6f; // old flat branch must not win
  float vertices[27]={};V3 expected[3];
  for(int i=0;i<3;++i){const V3 world={.12,.07,1.+i*2.};const V3 native=Apply(nativeView,world),in={game.xs*native.x/native.z,game.ys*native.y/native.z,game.a+game.b/native.z};vertices[i*9]=float(in.x);vertices[i*9+1]=float(in.y);vertices[i*9+2]=float(in.z);expected[i]=Apply(physicalEye,world);}
  D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(vb,0,D3D11_MAP_WRITE_DISCARD,0,&mapped)),"241 write native NDC particles");memcpy(mapped.pData,vertices,sizeof(vertices));context->Unmap(vb,0);
  context->UpdateSubresource(cb,0,nullptr,rows,0,0);context->SOSetTargets(1,&output,&offset);context->Draw(3,0);context->SOSetTargets(0,nullptr,nullptr);float got[12]={};Read(device,context,output,got);
  for(int i=0;i<3;++i){Check(fabs(got[i*4]/got[i*4+3]-game.xs*expected[i].x/expected[i].z)<1e-4&&fabs(got[i*4+1]/got[i*4+3]-game.ys*expected[i].y/expected[i].z)<1e-4,"241 executed flame shader keeps world depth through head turns and both eyes instead of UI panel");}
 }
 context->VSSetShader(original,nullptr,0);SafeRelease(original);ID3D11VertexShader* restored=nullptr;context->VSGetShader(&restored,nullptr,nullptr);Check(restored==vs,"241 original shader restored after both eyes");SafeRelease(restored);
 context->ClearState();SafeRelease(cb);SafeRelease(output);SafeRelease(gs);SafeRelease(layout);SafeRelease(vb);SafeRelease(vs);g_oCreateVertexShader=createVs;
 puts("PASS:241 actual flame shader stream-out, three head turns, near/far depth and both-eye disparity; UI shader exclusion and restored original VS, no second vertex transport");
}
