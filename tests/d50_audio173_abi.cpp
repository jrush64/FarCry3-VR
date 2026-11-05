// Standalone x86 ABI fixture only; never linked into proxy/runtime.
// Compile in an x86 MSVC environment: cl /nologo /W4 /EHsc tests/d50_audio173_abi.cpp
#include <windows.h>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#if !defined(_M_IX86)
#error This fixture verifies the installed x86 custom native ABI.
#endif
struct Reference173 {unsigned low,high;volatile LONG references;void* entity;};
struct Handle173 {unsigned low,high;};
static unsigned seenSound173=0,seenType173=0,calls173=0;
static Reference173* seenReference173=nullptr;
static void* seenRoute173=nullptr;
static bool invalid173=false;
static Handle173* __cdecl FakeBody173(unsigned sound,Handle173* out,unsigned soundType,Reference173* reference,void* route){
 seenSound173=sound;seenType173=soundType;seenReference173=reference;seenRoute173=route;++calls173;
 if(reference->references!=6)std::abort();
 out->low=invalid173?0xffffffffu:0x10203040;out->high=invalid173?0xffffffffu:0x50607080;
 InterlockedDecrement(&reference->references);return out;
}
__declspec(naked) static void FakeNative173(){
 __asm {
  push ebp
  mov ebp,esp
  push dword ptr [ebp+16]
  push dword ptr [ebp+12]
  push dword ptr [ebp+8]
  push edi
  push eax
  call FakeBody173
  add esp,20
  pop ebp
  ret
 }
}
// Retaining reference+8 matches the verified native caller contract. The
// supplied entry consumes that reference; the returned8-byte token is a value.
// This adapter is deliberately test-local: thread/owner/audio-role proof absent.
static bool InvokeFixture173(void* entry,unsigned sound,unsigned soundType,Reference173* reference,void* route,Handle173* output){
 if(!entry||!reference||!output)return false;
 InterlockedIncrement(&reference->references);
 Handle173* returned=nullptr;
 __asm {
  push edi
  mov edi,output
  mov eax,sound
  push route
  push reference
  push soundType
  call dword ptr [entry]
  add esp,12
  mov returned,eax
  pop edi
 }
 return returned==output;
}
static void Check173(bool value,const char* label){if(!value){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
int main(){
 static_assert(sizeof(Handle173)==8,"native playback token is two words");
 static_assert(offsetof(Reference173,references)==8,"native entity reference count offset");
 Reference173 reference={0x123,0x456,5,reinterpret_cast<void*>(0x12340000)};
 struct Guarded{unsigned before;Handle173 value;unsigned after;} output={0xabcdef12,{},0xdeadbeef};
 unsigned route=0x9876;
 volatile unsigned stackCanary=0x87654321;
 Check173(InvokeFixture173(reinterpret_cast<void*>(FakeNative173),0x173068,15,&reference,&route,&output.value),"custom result pointer preserved");
 Check173(seenSound173==0x173068&&seenType173==15&&seenReference173==&reference&&seenRoute173==&route,"EAX/EDI/three stack arguments mapped");
 Check173(reference.references==5&&calls173==1,"one retain consumed exactly once");
 Check173(output.value.low==0x10203040&&output.value.high==0x50607080&&output.before==0xabcdef12&&output.after==0xdeadbeef&&stackCanary==0x87654321,"eight-byte output and caller stack boundaries");
 invalid173=true;Check173(InvokeFixture173(reinterpret_cast<void*>(FakeNative173),0xffffffffu,15,&reference,nullptr,&output.value),"invalid-token path still returns caller storage");
 Check173(reference.references==5&&output.value.low==0xffffffffu&&output.value.high==0xffffffffu,"invalid output still consumes retained reference");
 Check173(!InvokeFixture173(nullptr,0,0,&reference,nullptr,&output.value)&&reference.references==5,"absent entry never retains reference");
 puts("PASS: isolated x86 scalar-sound ABI adapter; no game functions called, no live ownership/thread/audio proof");return 0;
}
