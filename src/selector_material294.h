#pragma once
#include "dxbc_patch.h"
#include <cstring>
namespace fc3material294 {
inline uint32_t Word(const std::vector<BYTE>& b,size_t p){uint32_t v=0;std::memcpy(&v,b.data()+p,4);return v;}
// FC3's deferred materials encode albedo with sqrt at target0. The XR sRGB
// render target performs display encoding itself. Retain the original material
// calculation, textures, masks and discards, replacing ONLY that final encoding.
// This never patches the game's bound shader, only the selector's private copy.
inline bool LinearAlbedo(std::vector<BYTE>& bytes){
 if(bytes.size()<36||std::memcmp(bytes.data(),"DXBC",4)||Word(bytes,24)!=bytes.size())return false;
 const uint32_t chunks=Word(bytes,28);if(chunks>(bytes.size()-32)/4)return false;
 std::vector<size_t> edits;
 for(uint32_t c=0;c<chunks;++c){const size_t chunk=Word(bytes,32+c*4);if(chunk>bytes.size()-8)return false;
  if(std::memcmp(bytes.data()+chunk,"SHDR",4)&&std::memcmp(bytes.data()+chunk,"SHEX",4))continue;
  const size_t size=Word(bytes,chunk+4),begin=chunk+8;if(size<8||size%4||size>bytes.size()-begin)return false;
  if((Word(bytes,begin)>>16)!=0||Word(bytes,begin+4)!=size/4)return false;
  for(size_t p=begin+8;p<begin+size;){const uint32_t op=Word(bytes,p);size_t words=(op>>24)&127;
   if((op&2047)==53){if(p+8>begin+size)return false;words=Word(bytes,p+4);}
   if(!words||words>(begin+size-p)/4)return false;
   // sqrt o0.xyz, source: exact immediate output0 RGB destination. Other
   // square roots (normals, distances, etc.) are left byte-for-byte unchanged.
   if((op&2047)==75&&words>=5&&Word(bytes,p+4)==0x00102072&&Word(bytes,p+8)==0)edits.push_back(p);
   p+=words*4;
  }
 }
 if(edits.size()!=1)return false;
 const size_t p=edits[0];uint32_t op=(Word(bytes,p)&~2047u)|54u;std::memcpy(bytes.data()+p,&op,4);
 uint32_t checksum[4];dxbc::ComputeChecksum(bytes.data(),bytes.size(),checksum);std::memcpy(bytes.data()+4,checksum,16);return true;
}
}
