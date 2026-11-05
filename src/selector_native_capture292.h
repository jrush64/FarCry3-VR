#pragma once
// Private, instance-derived visual snapshots. No asset-name substitution, native
// entity creation, inventory writes, or changes to the game's render context.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include "selector_contact292.h"
#include "selector_material294.h"
namespace fc3visual292 {
template<class T> inline void Drop(T*& p){if(p)p->Release();p=nullptr;}
struct Part {
 ID3D11Buffer* vertices=nullptr; ID3D11InputLayout* layout=nullptr;
 ID3D11Query* statistics=nullptr;
 ID3D11VertexShader* place=nullptr; ID3D11PixelShader* material=nullptr;
 ID3D11Buffer* constants[14]={}; ID3D11ShaderResourceView* textures[128]={}; ID3D11SamplerState* samplers[16]={};
 UINT count=0,stride=0,start=0,vertexBytes=0,indexBytes=0,positionOffset=0; unsigned long long shader=0,materialKey=0;
 float clipToGrip[16]={}; bool ready=false,hand=false,linearAlbedo=false;
 std::vector<BYTE> placeCode,materialCode;
 struct Input {UINT index,format,offset;};std::vector<Input> inputs;
 Part()=default;Part(const Part&)=delete;Part& operator=(const Part&)=delete;
 ~Part(){Drop(vertices);Drop(statistics);Drop(layout);Drop(place);Drop(material);for(auto*& p:constants)Drop(p);for(auto*& p:textures)Drop(p);for(auto*& p:samplers)Drop(p);}
};
inline bool Bytecode(ID3D11DeviceChild* shader,const GUID& guid,std::vector<BYTE>& bytes){
 UINT n=0;if(!shader||FAILED(shader->GetPrivateData(guid,&n,nullptr))||!n||n>65536)return false;
 bytes.resize(n);return SUCCEEDED(shader->GetPrivateData(guid,&n,bytes.data()));
}
inline DXGI_FORMAT Format(UINT n,D3D_REGISTER_COMPONENT_TYPE t){
 const DXGI_FORMAT f[3][4]={{DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32G32_FLOAT,DXGI_FORMAT_R32G32B32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT},{DXGI_FORMAT_R32_UINT,DXGI_FORMAT_R32G32_UINT,DXGI_FORMAT_R32G32B32_UINT,DXGI_FORMAT_R32G32B32A32_UINT},{DXGI_FORMAT_R32_SINT,DXGI_FORMAT_R32G32_SINT,DXGI_FORMAT_R32G32B32_SINT,DXGI_FORMAT_R32G32B32A32_SINT}};
 return n&&n<=4?f[t==D3D_REGISTER_COMPONENT_UINT32?1:t==D3D_REGISTER_COMPONENT_SINT32?2:0][n-1]:DXGI_FORMAT_UNKNOWN;
}
inline unsigned long long MaterialKey294(ID3D11DeviceContext* context){
 ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,0);
 unsigned long long hash=14695981039346656037ull;
 const auto mix=[&](const void* p){const auto value=reinterpret_cast<uintptr_t>(p);hash^=value;hash*=1099511628211ull;};mix(ps);Drop(ps);
 ID3D11ShaderResourceView* views[128]={};context->PSGetShaderResources(0,128,views);for(auto*& p:views){mix(p);Drop(p);}return hash;
}
// Run the ORIGINAL vertex shader through D3D11 stream output, preserving every
// output interpolator. Original material shader + actual bound textures and CB
// snapshots then reproduce the owned instance, including separately drawn parts.
inline bool Capture(ID3D11Device* device,ID3D11DeviceContext* source,UINT count,UINT start,INT base,
                    const GUID& codeGuid,Part& out,std::string& reason){
 if(out.ready||!device||!source||!count||count>300000||count%3){reason="unsupported triangle count";return false;}
 D3D11_PRIMITIVE_TOPOLOGY topology;source->IAGetPrimitiveTopology(&topology);
 if(topology!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST){reason="not triangle list";return false;}
 ID3D11GeometryShader* gs=nullptr;ID3D11HullShader* hs=nullptr;ID3D11DomainShader* ds=nullptr;source->GSGetShader(&gs,nullptr,nullptr);source->HSGetShader(&hs,nullptr,nullptr);source->DSGetShader(&ds,nullptr,nullptr);const bool extraStages=gs||hs||ds;Drop(gs);Drop(hs);Drop(ds);if(extraStages){reason="native draw uses an unsupported extra geometry stage";return false;}
 ID3D11VertexShader* gameVs=nullptr;ID3D11PixelShader* gamePs=nullptr;
 source->VSGetShader(&gameVs,nullptr,nullptr);source->PSGetShader(&gamePs,nullptr,nullptr);
 std::vector<BYTE> vsCode,psCode;bool ok=Bytecode(gameVs,codeGuid,vsCode)&&Bytecode(gamePs,codeGuid,psCode);Drop(gameVs);Drop(gamePs);
 if(!ok){reason="original shader bytecode unavailable";return false;}
 ID3D11ShaderReflection *vsReflection=nullptr,*psReflection=nullptr;
 if(FAILED(D3DReflect(vsCode.data(),vsCode.size(),IID_ID3D11ShaderReflection,(void**)&vsReflection))||
    FAILED(D3DReflect(psCode.data(),psCode.size(),IID_ID3D11ShaderReflection,(void**)&psReflection))){Drop(vsReflection);Drop(psReflection);reason="reflection failed";return false;}
 D3D11_SHADER_DESC vd={},pd={};vsReflection->GetDesc(&vd);psReflection->GetDesc(&pd);
 bool cbUsed[14]={},textureUsed[128]={},samplerUsed[16]={};
 bool colour=false,writesDepth=false;for(UINT i=0;i<pd.OutputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC d={};psReflection->GetOutputParameterDesc(i,&d);if(d.SystemValueType==D3D_NAME_TARGET&&d.SemanticIndex==0&&(d.Mask&7)==7)colour=true;if(d.SystemValueType==D3D_NAME_DEPTH)writesDepth=true;}
 if(writesDepth)colour=false;
 for(UINT i=0;i<pd.BoundResources;++i){D3D11_SHADER_INPUT_BIND_DESC d={};psReflection->GetResourceBindingDesc(i,&d);if(d.Type>=D3D_SIT_UAV_RWTYPED)colour=false;
  const UINT limit=d.Type==D3D_SIT_CBUFFER?14:d.Type==D3D_SIT_SAMPLER?16:128;if(d.BindPoint>=limit||!d.BindCount||d.BindCount>limit-d.BindPoint){colour=false;continue;}
  for(UINT j=d.BindPoint;j<d.BindPoint+d.BindCount;++j){if(d.Type==D3D_SIT_CBUFFER)cbUsed[j]=true;else if(d.Type==D3D_SIT_SAMPLER)samplerUsed[j]=true;else textureUsed[j]=true;}
 }
 Drop(psReflection);
 if(!colour||vd.OutputParameters>32){Drop(vsReflection);reason="material is not a supported colour pass";return false;}
 if(pd.OutputParameters>=3)out.linearAlbedo=fc3material294::LinearAlbedo(psCode);
 std::vector<D3D11_SIGNATURE_PARAMETER_DESC> sig(vd.OutputParameters);std::vector<std::string> names;names.reserve(vd.OutputParameters);
 std::vector<D3D11_SO_DECLARATION_ENTRY> so;std::vector<D3D11_INPUT_ELEMENT_DESC> input;
 std::string ins="struct I {",outs="struct O {",body="O main(I i) {O o;";UINT stride=0;int position=-1;
 for(UINT i=0;i<vd.OutputParameters;++i){auto& s=sig[i];vsReflection->GetOutputParameterDesc(i,&s);
  UINT first=0;while(first<4&&!(s.Mask&(1u<<first)))++first;UINT n=0;while(first+n<4&&(s.Mask&(1u<<(first+n))))++n;
  if(!n||s.Mask!=(((1u<<n)-1)<<first)){ok=false;break;}
  names.push_back(s.SemanticName);const std::string field="v"+std::to_string(i),type=(s.ComponentType==D3D_REGISTER_COMPONENT_UINT32?"uint":s.ComponentType==D3D_REGISTER_COMPONENT_SINT32?"int":"float")+std::to_string(n);
  ins+=type+" "+field+":CAPTURE"+std::to_string(i)+";";
  const std::string scalar=s.ComponentType==D3D_REGISTER_COMPONENT_UINT32?"uint":s.ComponentType==D3D_REGISTER_COMPONENT_SINT32?"int":"float";
  outs+=(first?scalar+"4":type)+" "+field+":"+names.back()+std::to_string(s.SemanticIndex)+";";
  if(first){body+="o."+field+"=0;o."+field+"."+std::string("xyzw"+first,n)+"=i."+field+";";}else body+="o."+field+"=i."+field+";";
  so.push_back({0,names.back().c_str(),s.SemanticIndex,BYTE(first),BYTE(n),0});
  input.push_back({"CAPTURE",i,Format(n,s.ComponentType),0,stride,D3D11_INPUT_PER_VERTEX_DATA,0});if(s.SystemValueType==D3D_NAME_POSITION)out.positionOffset=stride;stride+=n*4;
  if(s.SystemValueType==D3D_NAME_POSITION&&n==4&&s.ComponentType==D3D_REGISTER_COMPONENT_FLOAT32)position=int(i);
 }
 if(!ok||position<0||stride>512||UINT64(stride)*count>32*1024*1024){Drop(vsReflection);reason="unsupported native output signature";return false;}
 // Reflection-owned semantic strings remain alive until SO shader creation.
 const std::string pos="i.v"+std::to_string(position);body+="o.v"+std::to_string(position)+"=float4(dot(R[0],"+pos+"),dot(R[1],"+pos+"),dot(R[2],"+pos+"),dot(R[3],"+pos+"));return o;}";
 const std::string program="cbuffer Place:register(b0){float4 R[4];};"+ins+"};"+outs+"};"+body;
 ID3DBlob *compiled=nullptr,*errors=nullptr;HRESULT hr=D3DCompile(program.data(),program.size(),nullptr,nullptr,nullptr,"main","vs_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&compiled,&errors);
 if(FAILED(hr)){if(errors)reason.assign((char*)errors->GetBufferPointer(),errors->GetBufferSize());Drop(errors);Drop(vsReflection);return false;}Drop(errors);
 ID3D11GeometryShader* stream=nullptr;ID3D11VertexShader* original=nullptr;
 out.placeCode.assign((BYTE*)compiled->GetBufferPointer(),(BYTE*)compiled->GetBufferPointer()+compiled->GetBufferSize());out.materialCode=psCode;
 for(const auto& e:input)out.inputs.push_back({e.SemanticIndex,UINT(e.Format),e.AlignedByteOffset});
 ok=SUCCEEDED(device->CreateVertexShader(vsCode.data(),vsCode.size(),nullptr,&original))&&
    SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(vsCode.data(),vsCode.size(),so.data(),UINT(so.size()),&stride,1,D3D11_SO_NO_RASTERIZED_STREAM,nullptr,&stream))&&
    SUCCEEDED(device->CreateVertexShader(compiled->GetBufferPointer(),compiled->GetBufferSize(),nullptr,&out.place))&&
    SUCCEEDED(device->CreateInputLayout(input.data(),UINT(input.size()),compiled->GetBufferPointer(),compiled->GetBufferSize(),&out.layout))&&
    SUCCEEDED(device->CreatePixelShader(psCode.data(),psCode.size(),nullptr,&out.material));Drop(compiled);Drop(vsReflection);
 D3D11_BUFFER_DESC desc={};desc.ByteWidth=stride*count;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_STREAM_OUTPUT|D3D11_BIND_VERTEX_BUFFER;
 ok=ok&&SUCCEEDED(device->CreateBuffer(&desc,nullptr,&out.vertices));
 D3D11_QUERY_DESC query={D3D11_QUERY_SO_STATISTICS,0};ok=ok&&SUCCEEDED(device->CreateQuery(&query,&out.statistics));
 ID3D11DeviceContext* commands=nullptr;ok=ok&&SUCCEEDED(device->CreateDeferredContext(0,&commands));
 if(ok){
  ID3D11InputLayout* layout=nullptr;source->IAGetInputLayout(&layout);commands->IASetInputLayout(layout);Drop(layout);
  ID3D11Buffer* vb[32]={};UINT strides[32]={},offsets[32]={};source->IAGetVertexBuffers(0,32,vb,strides,offsets);commands->IASetVertexBuffers(0,32,vb,strides,offsets);for(auto*& p:vb)Drop(p);
  ID3D11Buffer* ib=nullptr;DXGI_FORMAT fmt;UINT offset=0;source->IAGetIndexBuffer(&ib,&fmt,&offset);commands->IASetIndexBuffer(ib,fmt,offset);Drop(ib);commands->IASetPrimitiveTopology(topology);
  ID3D11Buffer* cb[14]={};source->VSGetConstantBuffers(0,14,cb);commands->VSSetConstantBuffers(0,14,cb);for(auto*& p:cb)Drop(p);
  ID3D11ShaderResourceView* textures[128]={};source->VSGetShaderResources(0,128,textures);commands->VSSetShaderResources(0,128,textures);for(auto*& p:textures)Drop(p);
  ID3D11SamplerState* samplers[16]={};source->VSGetSamplers(0,16,samplers);commands->VSSetSamplers(0,16,samplers);for(auto*& p:samplers)Drop(p);
  commands->VSSetShader(original,nullptr,0);commands->GSSetShader(stream,nullptr,0);UINT zero=0;commands->SOSetTargets(1,&out.vertices,&zero);commands->Begin(out.statistics);commands->DrawIndexed(count,start,base);commands->End(out.statistics);commands->SOSetTargets(0,nullptr,nullptr);
  ID3D11CommandList* list=nullptr;ok=SUCCEEDED(commands->FinishCommandList(FALSE,&list));if(ok){source->ExecuteCommandList(list,TRUE);Drop(list);}
  source->PSGetConstantBuffers(0,14,cb);
  for(UINT i=0;i<14&&ok;++i)if(cbUsed[i]){if(!cb[i]){ok=false;break;}D3D11_BUFFER_DESC d={};cb[i]->GetDesc(&d);if(d.ByteWidth>65536){ok=false;break;}d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;d.CPUAccessFlags=d.MiscFlags=d.StructureByteStride=0;ok=SUCCEEDED(device->CreateBuffer(&d,nullptr,&out.constants[i]));if(ok)source->CopyResource(out.constants[i],cb[i]);}
  for(auto*& p:cb)Drop(p);source->PSGetShaderResources(0,128,out.textures);source->PSGetSamplers(0,16,out.samplers);
  for(UINT i=0;i<128;++i)if(!textureUsed[i])Drop(out.textures[i]);for(UINT i=0;i<16;++i)if(!samplerUsed[i])Drop(out.samplers[i]);
 }
 Drop(commands);Drop(original);Drop(stream);out.count=count;out.stride=stride;out.start=start;out.ready=ok;
 if(!ok&&reason.empty())reason="native snapshot resource creation failed";return ok;
}
inline bool Geometry(ID3D11Device* device,ID3D11DeviceContext* context,Part& part,std::vector<fc3contact292::Triangle>& triangles){
 if(!part.ready||part.positionOffset+16>part.stride)return false;
 D3D11_BUFFER_DESC d={};part.vertices->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Buffer* copy=nullptr;
 if(FAILED(device->CreateBuffer(&d,nullptr,&copy)))return false;context->CopyResource(copy,part.vertices);D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(context->Map(copy,0,D3D11_MAP_READ,0,&map))){Drop(copy);return false;}
 D3D11_QUERY_DATA_SO_STATISTICS statistics={};if(!part.statistics||context->GetData(part.statistics,&statistics,sizeof(statistics),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK||!statistics.NumPrimitivesWritten||statistics.NumPrimitivesWritten!=statistics.PrimitivesStorageNeeded||statistics.NumPrimitivesWritten*3>part.count){context->Unmap(copy,0);Drop(copy);return false;}part.count=UINT(statistics.NumPrimitivesWritten*3);
 std::vector<fc3contact292::Triangle> result;result.reserve(part.count/3);bool valid=true;
 for(UINT i=0;i<part.count&&valid;i+=3){fc3contact292::P p[3];for(int j=0;j<3;++j){float raw[4],v[4]={};memcpy(raw,static_cast<const BYTE*>(map.pData)+(i+j)*part.stride+part.positionOffset,16);for(int r=0;r<4;++r)for(int c=0;c<4;++c)v[r]+=part.clipToGrip[r*4+c]*raw[c];if(!std::isfinite(v[3])||std::fabs(v[3])<1e-6f){valid=false;break;}p[j]={v[0]/v[3],v[1]/v[3],v[2]/v[3]};if(!std::isfinite(p[j].x)||!std::isfinite(p[j].y)||!std::isfinite(p[j].z)||std::fabs(p[j].x)>5||std::fabs(p[j].y)>5||std::fabs(p[j].z)>5){valid=false;break;}}if(valid)result.push_back({p[0],p[1],p[2]});}
 context->Unmap(copy,0);Drop(copy);if(!valid)return false;triangles.insert(triangles.end(),result.begin(),result.end());return true;
}
inline bool CropHand(ID3D11Device* device,ID3D11DeviceContext* context,Part& part){
 std::vector<fc3contact292::Triangle> triangles;if(!Geometry(device,context,part,triangles))return false;
 D3D11_BUFFER_DESC d={};part.vertices->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Buffer* copy=nullptr;
 if(FAILED(device->CreateBuffer(&d,nullptr,&copy)))return false;context->CopyResource(copy,part.vertices);D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(context->Map(copy,0,D3D11_MAP_READ,0,&map))){Drop(copy);return false;}
 std::vector<BYTE> cropped;
 for(size_t i=0;i<triangles.size();++i){const auto& t=triangles[i];const fc3contact292::P v[]={t.a,t.b,t.c};bool keep=(t.a.x+t.b.x+t.c.x)/3>=-.025f;for(const auto& p:v)keep=keep&&fc3contact292::Dot(p,p)<.23f*.23f;
  if(keep){const BYTE* first=static_cast<const BYTE*>(map.pData)+i*3*part.stride;cropped.insert(cropped.end(),first,first+3*part.stride);}
 }context->Unmap(copy,0);Drop(copy);if(cropped.empty())return false;
 d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;d.CPUAccessFlags=0;d.ByteWidth=UINT(cropped.size());D3D11_SUBRESOURCE_DATA init={cropped.data(),0,0};ID3D11Buffer* buffer=nullptr;if(FAILED(device->CreateBuffer(&d,&init,&buffer)))return false;
 Drop(part.vertices);part.vertices=buffer;part.count=UINT(cropped.size()/part.stride);return true;
}
}
