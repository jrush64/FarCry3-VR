// The captured 8CBC flame shader emits NDC with w=1. Generic patching
// classifies w=1 as UI and discards its depth. Use a cached depth-aware
// variant only for the exact native flamer particle draw; restore after both eyes.
const GUID kFlamerProjection241={0x241fc301,0x9a12,0x4c21,{0xa1,0x43,0x66,0x6c,0x61,0x6d,0x65,1}};
bool FlamerProjectedDraw241(ID3D11DeviceContext* context,const DrawCall& call){
 if(!FlamerModel()||!MotionInteractionEnabled()||!g_handRightValid||!fc3pad::Latest().active||fc3pad::GameplayInputBlocked()||call.kind!=1||call.a>16384)return false;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const bool shader=ShaderHash(vs)==0x8CBC4FD2A59FD8A3ull;SafeRelease(vs);if(!shader)return false;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);
 return d.Usage==D3D11_USAGE_DYNAMIC&&d.ByteWidth==147456&&stride==36&&offset==0;
}
bool BeginFlamerProjection241(ID3D11DeviceContext* context,ID3D11VertexShader*& original){
 original=nullptr;ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);if(!vs)return false;
 ID3D11VertexShader* corrected=nullptr;UINT bytes=sizeof(corrected);
 if(FAILED(vs->GetPrivateData(kFlamerProjection241,&bytes,&corrected))||!corrected){
  UINT size=0;std::vector<BYTE> code;std::vector<uint8_t> patched;
  if(FAILED(vs->GetPrivateData(kEjectionBytecodeGuid,&size,nullptr))||!size||size>65536){SafeRelease(vs);return false;}code.resize(size);
  if(FAILED(vs->GetPrivateData(kEjectionBytecodeGuid,&size,code.data()))||dxbc::PatchProjectedVertexShader241(code.data(),size,patched)!=dxbc::Result::Patched||!g_oCreateVertexShader||FAILED(g_oCreateVertexShader(g_dev,patched.data(),patched.size(),nullptr,&corrected))){SafeRelease(vs);return false;}
  vs->SetPrivateDataInterface(kFlamerProjection241,corrected);
 }
 context->VSSetShader(corrected,nullptr,0);SafeRelease(corrected);original=vs;
 static UINT logs=0;if(logs++<4)Log("[FLAME241] exact projected stream uses world-depth eye reprojection; native birth owns muzzle and spray, no second vertex transport");
 return true;
}
