// Exact captured soft-particle PS families. VS supplies eye depth UV; convert
// the sampled point back to native clip depth before the unchanged native
// depth-scale/plane/fade math. World positions and authored fade varyings stay native.
PixelResult PatchParticleDepth254(const uint8_t* src,const ContainerChunk* chunks,uint32_t chunkCount,int shaderChunk,const std::vector<uint32_t>& t,std::vector<uint8_t>& out,bool msaa=false){
 size_t first=2,tempAt=0;uint32_t temps=0;
 while(first<t.size()){
  const auto op=t[first]&0x7ff;const auto len=InstructionLength(t.data()+first,t.size()-first);
  if(!len||first+len>t.size())return PixelResult::Malformed;
  if(!IsDeclaration(op))break;
  if(op==kOpDclConstantBuffer&&len==4&&t[first+2]==kEyeCbSlot)return PixelResult::Unsupported;
  if(op==kOpDclTemps&&len==2){tempAt=first+1;temps=t[first+1];}first+=len;
 }
 if(!tempAt||first==t.size())return PixelResult::Unsupported;
 std::vector<uint32_t> p(t.begin(),t.begin()+first);p[tempAt]=temps+2;
 const uint32_t cb[]={Op(kOpDclConstantBuffer,4),0x00208E46u,kEyeCbSlot,4};p.insert(p.begin()+2,cb,cb+4);
 const uint32_t C=temps,G=temps+1;int hits=0;std::vector<uint32_t> b;
 for(size_t i=first;i<t.size();){
  const auto len=InstructionLength(t.data()+i,t.size()-i);if(!len||i+len>t.size())return PixelResult::Malformed;
  const size_t end=i+len;p.insert(p.end(),t.begin()+i,t.begin()+end);
  if((t[i]&0x7ff)==(msaa?46u:69u)){ // ld_ms / sample, including extended opcodes
   size_t a=i+1;uint32_t extended=t[i];while(extended&0x80000000u){if(a>=end)return PixelResult::Malformed;extended=t[a++];}
   const auto dst=a;const auto dl=OperandLength(t.data(),a,end);if(!dl)return PixelResult::Unsupported;a+=dl;
   const auto uv=OperandLength(t.data(),a,end);if(!uv)return PixelResult::Unsupported;a+=uv;
   if(a+1<end&&((t[a]>>12)&255)==7&&t[a+1]==(msaa?1u:0u)){
    const uint32_t mask=(t[dst]>>4)&15;
    if(dl!=2||((t[dst]>>12)&255)!=kOperandTemp||!mask||(mask&(mask-1)))return PixelResult::Unsupported;
    uint32_t component=0;while((1u<<component)!=mask)++component;
    const uint32_t reg=t[dst+1];++hits;
    b.clear();DstTemp(b,C,3);SrcInput(b,5,Swz(0,1,0,0));SrcInput(b,5,Swz(2,2,2,2));Emit(p,kOpDiv,b);
    b.clear();DstTemp(b,C,3);SrcTemp(b,C,Swz(0,1,0,0));SrcImm(b,F(2),F(-2),0,0);SrcImm(b,F(-1),F(1),0,0);Emit(p,kOpMad,b);
    b.clear();DstTemp(b,C,4);SrcTemp(b,reg,Swz(component,component,component,component));Emit(p,54,b);
    b.clear();DstTemp(b,C,8);SrcImm(b,F(1),F(1),F(1),F(1));Emit(p,54,b);
    for(uint32_t col=0;col<4;++col){b.clear();DstTemp(b,G,15);SrcCb(b,kEyeCbSlot,col,Swz(0,1,2,3));SrcTemp(b,C,Swz(col,col,col,col));SrcTemp(b,col?G:C,Swz(0,1,2,3));Emit(p,kOpMad,b);}
    b.clear();DstTemp(b,reg,mask);SrcTemp(b,G,Swz(2,2,2,2));SrcTemp(b,G,Swz(3,3,3,3));Emit(p,kOpDiv,b);
   }
  }i=end;
 }
 if(hits!=(msaa?4:1))return PixelResult::Unsupported;p[1]=static_cast<uint32_t>(p.size());
 size_t total=32+4*chunkCount;for(uint32_t c=0;c<chunkCount;++c)total+=8+(int(c)==shaderChunk?p.size()*4:chunks[c].size);
 out.assign(total,0);memcpy(out.data(),src,32);const uint32_t bytes=static_cast<uint32_t>(total);memcpy(out.data()+24,&bytes,4);size_t cursor=32+4*chunkCount;
 for(uint32_t c=0;c<chunkCount;++c){const uint32_t offset=static_cast<uint32_t>(cursor),size=int(c)==shaderChunk?static_cast<uint32_t>(p.size()*4):chunks[c].size;memcpy(out.data()+32+4*c,&offset,4);memcpy(out.data()+cursor,&chunks[c].tag,4);memcpy(out.data()+cursor+4,&size,4);memcpy(out.data()+cursor+8,int(c)==shaderChunk?static_cast<const void*>(p.data()):chunks[c].data,size);cursor+=8+size;}
 uint32_t checksum[4];ComputeChecksum(out.data(),out.size(),checksum);memcpy(out.data()+4,checksum,16);return PixelResult::Patched;
}
