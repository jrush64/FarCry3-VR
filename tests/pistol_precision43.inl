#include "pistol_split_native43.inl"
#include "pistol_split_capture43.inl"
void TestPistolPrecision43(ID3D11Device* d,ID3D11DeviceContext* c){
 const LONG profile=g_weaponMechanicProfile,mechanicFrame=g_weaponMechanicFrame;g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;
 auto matrix=[](const double* x){Mat4 m;memcpy(m.m,x,sizeof(m.m));return m;};
 auto legacy=[](float* g,const Mat4& w){for(int r=0;r<3;++r)for(int k=0;k<4;++k)g[r*4+k]=float(w.m[r*4+k]-(k==3?g[12+r]:0));};
 float invalid[20];memcpy(invalid,kSplitCapture43[0].parent,sizeof(invalid));float before[20];memcpy(before,invalid,sizeof(before));Mat4 bad=matrix(kSplitCapture43[0].desiredParent);bad.m[3]=NAN;Check(!WritePistolSplitWorld(invalid,bad)&&memcmp(before,invalid,sizeof(before))==0,"invalid precision input leaves original CB unchanged");
 ID3D11VertexShader* vs[2]={};Check(SUCCEEDED(d->CreateVertexShader(kSplitRigidVS43,sizeof(kSplitRigidVS43),nullptr,&vs[0])),"native captured rigid shader");Check(SUCCEEDED(d->CreateVertexShader(kSplitLensVS43,sizeof(kSplitLensVS43),nullptr,&vs[1])),"native captured lens cb4 shader");
 const char* gsText="struct V{float4 p:SV_Position;};[maxvertexcount(1)]void gs(point V v[1],inout PointStream<V>s){s.Append(v[0]);}";
 ID3DBlob* code=nullptr;Check(SUCCEEDED(D3DCompile(gsText,strlen(gsText),nullptr,nullptr,nullptr,"gs","gs_5_0",0,0,&code,nullptr)),"precision stream-out GS");D3D11_SO_DECLARATION_ENTRY entry={0,"SV_Position",0,0,4,0};UINT stride=16;ID3D11GeometryShader* gs=nullptr;Check(SUCCEEDED(d->CreateGeometryShaderWithStreamOutput(code->GetBufferPointer(),code->GetBufferSize(),&entry,1,&stride,1,D3D11_SO_NO_RASTERIZED_STREAM,nullptr,&gs)),"actual native VS output readback");SafeRelease(code);
 D3D11_INPUT_ELEMENT_DESC rigid[]={
  {"position",0,DXGI_FORMAT_R32G32B32A32_SINT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"texcoord",0,DXGI_FORMAT_R32G32_SINT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"texcoord",1,DXGI_FORMAT_R32G32_SINT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"normal",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,48,D3D11_INPUT_PER_VERTEX_DATA,0},{"color",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,64,D3D11_INPUT_PER_VERTEX_DATA,0},{"fog",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,80,D3D11_INPUT_PER_VERTEX_DATA,0},{"psize",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,96,D3D11_INPUT_PER_VERTEX_DATA,0}};
 D3D11_INPUT_ELEMENT_DESC lens[]={
  {"position",0,DXGI_FORMAT_R32G32B32A32_SINT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"texcoord",0,DXGI_FORMAT_R32G32_SINT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"blendweight",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"blendindices",0,DXGI_FORMAT_R32G32B32A32_UINT,0,48,D3D11_INPUT_PER_VERTEX_DATA,0},{"normal",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,64,D3D11_INPUT_PER_VERTEX_DATA,0},{"color",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,80,D3D11_INPUT_PER_VERTEX_DATA,0}};
 ID3D11InputLayout* layouts[2]={};Check(SUCCEEDED(d->CreateInputLayout(rigid,7,kSplitRigidVS43,sizeof(kSplitRigidVS43),&layouts[0])),"rigid input signature");Check(SUCCEEDED(d->CreateInputLayout(lens,6,kSplitLensVS43,sizeof(kSplitLensVS43),&layouts[1])),"lens input signature");
 float vertices[2][28]={};vertices[0][14]=1;vertices[0][20]=1;vertices[0][25]=1;vertices[1][8]=1;vertices[1][18]=1;ID3D11Buffer* vb[2]={};
 for(int i=0;i<2;++i){D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(vertices[i]);b.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA init={vertices[i],0,0};Check(SUCCEEDED(d->CreateBuffer(&b,&init,&vb[i])),"native shader input vertex");}
 auto buffer=[&](UINT bytes,UINT bind){D3D11_BUFFER_DESC b={};b.ByteWidth=bytes;b.BindFlags=bind;ID3D11Buffer* p=nullptr;Check(SUCCEEDED(d->CreateBuffer(&b,nullptr,&p)),"precision GPU buffer");return p;};
 ID3D11Buffer* cb0=buffer(80,D3D11_BIND_CONSTANT_BUFFER),*cb1=buffer(53*16,D3D11_BIND_CONSTANT_BUFFER),*zero=buffer(16,D3D11_BIND_CONSTANT_BUFFER),*bones=buffer(2304,D3D11_BIND_CONSTANT_BUFFER),*out=buffer(16,D3D11_BIND_STREAM_OUTPUT);
 float zeros[4]={},identityBones[576]={};for(int b=0;b<48;++b)identityBones[b*12]=identityBones[b*12+5]=identityBones[b*12+10]=1;c->UpdateSubresource(zero,0,nullptr,zeros,0,0);c->UpdateSubresource(bones,0,nullptr,identityBones,0,0);
 D3D11_BUFFER_DESC sd={};out->GetDesc(&sd);sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Buffer* staging=nullptr;Check(SUCCEEDED(d->CreateBuffer(&sd,nullptr,&staging)),"precision staging readback");
 ID3D11DeviceContext* deferred=nullptr;Check(SUCCEEDED(d->CreateDeferredContext(0,&deferred)),"precision deferred ownership replay");
 auto draw=[&](ID3D11DeviceContext* ctx,int shader,const float* g,const float* camera,float* p){
  ctx->UpdateSubresource(cb0,0,nullptr,g,0,0);ctx->UpdateSubresource(cb1,0,nullptr,camera,0,0);ID3D11Buffer* cb[]={cb0,cb1,zero,zero,bones};ctx->VSSetConstantBuffers(0,5,cb);ctx->VSSetShader(vs[shader],nullptr,0);ctx->GSSetShader(gs,nullptr,0);ctx->IASetInputLayout(layouts[shader]);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);UINT vstride=112,off=0;ctx->IASetVertexBuffers(0,1,&vb[shader],&vstride,&off);ctx->SOSetTargets(1,&out,&off);ctx->Draw(1,0);ctx->SOSetTargets(0,nullptr,nullptr);
  if(ctx==deferred){ID3D11CommandList* list=nullptr;Check(SUCCEEDED(ctx->FinishCommandList(FALSE,&list)),"precision deferred finish");c->ExecuteCommandList(list,FALSE);SafeRelease(list);}
  c->CopyResource(staging,out);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(c->Map(staging,0,D3D11_MAP_READ,0,&map)),"actual executed native shader result");memcpy(p,map.pData,16);c->Unmap(staging,0);for(int i=0;i<4;++i)Check(std::isfinite(p[i]),"native position is finite");
 };
 double worstCPU=0,worstGPU=0,worstOld=0;int pairs=0,lensPairs=0,deferredPairs=0;
 for(const auto& r:kSplitCapture43){
  float oldParent[20],oldChild[20],newParent[20],newChild[20];memcpy(oldParent,r.parent,80);memcpy(oldChild,r.child,80);memcpy(newParent,r.parent,80);memcpy(newChild,r.child,80);
  const Mat4 parent=matrix(r.desiredParent),child=matrix(r.desiredChild);legacy(oldParent,parent);legacy(oldChild,child);WritePistolWorld(newParent,parent);WritePistolWorld(newChild,child);
  for(int i=15;i<20;++i)Check(newParent[i]==r.parent[i]&&newChild[i]==r.child[i],"packing preserves shader padding/UV/decode fields");
  for(int row=0;row<3;++row){const double reconstructed=double(newChild[row*4+3])+newChild[12+row];worstCPU=(std::max)(worstCPU,std::fabs(reconstructed-child.m[row*4+3]));Check(newChild[row*4+3]==float(child.m[row*4+3]),"lighting high coordinate stays nearest corrected position");}
  float camera[53*4]={};camera[0]=camera[5]=camera[10]=camera[15]=1;camera[52*4+1]=1;for(int i=0;i<3;++i)camera[38*4+i]=r.parent[i*4+3];
  float oldP[4],oldC[4],newP[4],newC[4];draw(c,0,oldParent,camera,oldP);draw(c,r.lens,oldChild,camera,oldC);draw(c,0,newParent,camera,newP);draw(c,r.lens,newChild,camera,newC);
  for(int i=0;i<3;++i){const double want=child.m[i*4+3]-parent.m[i*4+3];worstGPU=(std::max)(worstGPU,std::fabs(double(newC[i])-newP[i]-want));worstOld=(std::max)(worstOld,std::fabs(double(oldC[i])-oldP[i]-want));}
  if(pairs<16){draw(deferred,0,newParent,camera,newP);draw(deferred,r.lens,newChild,camera,newC);for(int i=0;i<3;++i)Check(std::fabs(double(newC[i])-newP[i]-(child.m[i*4+3]-parent.m[i*4+3]))<2e-6,"deferred executed buffers preserve precision and draw ownership");++deferredPairs;}
  ++pairs;lensPairs+=r.lens;
 }
 Check(pairs==1123&&lensPairs==224&&worstCPU<1e-10&&worstGPU<2e-6&&worstOld>5e-5,"real captures reproduce old quantization and new native GPU precision recovery");
 printf("PASS: precision43 %d real capture pairs (%d cb4-lens), %d deferred pairs; worst legacy GPU %.6f mm -> corrected %.6f mm, CPU %.9f mm\n",pairs,lensPairs,deferredPairs,worstOld*1000,worstGPU*1000,worstCPU*1000);
 g_weaponMechanicProfile=12;float other[20],expected[20];memcpy(other,kSplitCapture43[0].parent,80);memcpy(expected,other,80);const Mat4 otherWorld=matrix(kSplitCapture43[0].desiredChild);legacy(expected,otherWorld);WritePistolWorld(other,otherWorld);Check(memcmp(other,expected,80)==0,"non1911 output is byte-identical legacy path");g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame-3;memcpy(other,kSplitCapture43[0].parent,80);WritePistolWorld(other,otherWorld);Check(memcmp(other,expected,80)==0,"stale weapon profile cannot change packing");
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanicFrame;c->ClearState();deferred->ClearState();SafeRelease(deferred);SafeRelease(staging);SafeRelease(out);SafeRelease(cb0);SafeRelease(cb1);SafeRelease(zero);SafeRelease(bones);SafeRelease(gs);for(int i=0;i<2;++i){SafeRelease(vb[i]);SafeRelease(layouts[i]);SafeRelease(vs[i]);}
}
