// Read-only native bow / repair-tool sampling before any motion override.
struct Weapons8Capture{LONG kind=-1,seenFrame=-1,lastSeen=-100000;ULONGLONG renewal=0;UINT seen[32]={},seenCount=0;LONG records[2]={};bool trigger=false;}g_weapons8Capture;
int Weapons8Geometry(const fc3weapon::Geometry& g){if(g.stride!=40||g.offset||g.format!=57)return -1;return g.vertexBytes==230760&&g.indexBytes==44856?0:g.vertexBytes==213320&&g.indexBytes==34200?1:-1;}
void ObserveWeapons8Capture(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(!g_animation.enabled||g_pistolMenu.open||call.kind!=0||call.d!=0)return;
 const auto geo=NativeWeaponGeometry(context,call);const int k=Weapons8Geometry(geo);if(k<0)return;
 auto& c=g_weapons8Capture;const auto now=GetTickCount64();const bool changed=c.kind!=k;c.kind=k;c.lastSeen=g_frame;
 if(changed||now-c.renewal>1500){c.renewal=now;g_animation.until=now+10000;g_animation.sample=true;}
 if(c.seenFrame!=g_frame){c.seenFrame=g_frame;c.seenCount=0;}
 const UINT key=call.b^(call.a<<16);for(UINT i=0;i<c.seenCount;++i)if(c.seen[i]==key)return;if(c.seenCount>=32)return;c.seen[c.seenCount++]=key;
 AnimationRecord r;AnimationStamp(r);r.kind=19;r.side=-1;r.state=k;r.event=variant;r.drawKind=call.kind;r.count=call.a;r.start=call.b;r.base=call.d;r.vertexBytes=geo.vertexBytes;r.stride=geo.stride;r.offset=geo.offset;r.indexFormat=geo.format;r.stateWords[0]=geo.indexBytes;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);r.shader=ShaderHash(vs);SafeRelease(vs);
 ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,off=0;DXGI_FORMAT fmt;context->IAGetVertexBuffers(0,1,&vb,&stride,&off);context->IAGetIndexBuffer(&ib,&fmt,&off);r.vertex=reinterpret_cast<uintptr_t>(vb);r.index=reinterpret_cast<uintptr_t>(ib);SafeRelease(vb);SafeRelease(ib);
 ID3D11Buffer* cb[4]={};context->VSGetConstantBuffers(0,4,cb);ReadCopy(cb[0],r.globals,sizeof(r.globals));r.rawBytes=ReadCopy(cb[3],r.raw,sizeof(r.raw));CameraSummary camera={};UINT bytes=sizeof(camera);if(cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera)){memcpy(r.camera,camera.right,12);memcpy(r.camera+3,camera.up,12);memcpy(r.camera+6,camera.forward,12);memcpy(r.camera+9,camera.position,12);r.camera[12]=camera.xScale;r.camera[13]=camera.yScale;}for(auto*& b:cb)SafeRelease(b);
 const auto controls=fc3pad::Latest();const bool trigger=controls.rightTrigger>.2f;if(trigger!=c.trigger||changed){r.flags|=0x100;r.stateWords[1]=trigger?1:0;c.trigger=trigger;}
 const auto before=g_animation.totalBytes;AnimationStore(r);if(g_animation.totalBytes>before)++c.records[k];
 if(changed)Log("[WEAPONS8 CAPTURE] %s geometry verified: native parts, inputs and palettes recorded automatically",k==0?"recurve bow":"repair tool");
}
bool Weapons8CaptureNote(){const auto& c=g_weapons8Capture;if(c.kind<0||g_frame-c.lastSeen>3)return false;wchar_t note[256];if(g_animation.error)_snwprintf_s(note,256,_TRUNCATE,L"CAPTURE FILE ERROR %ld\nStop and report this message.",g_animation.error);else if(g_animation.full)wcscpy_s(note,L"CAPTURE LIMIT REACHED\nQuit; captured data is saved.");else if(c.records[c.kind]==0)wcscpy_s(note,L"BOW / REPAIR TOOL - OPENING CAPTURE");else _snwprintf_s(note,256,_TRUNCATE,c.kind==0?L"BOW - CAPTURING (%ld records)\nUse normal aim/fire controls.\nFire 3 arrows at a wall or the ground.":L"REPAIR TOOL - CAPTURING (%ld records)\nHold RT for 3 seconds, release, repeat.\nThen repair a damaged vehicle if available.",c.records[c.kind]);fc3xr::SetTestNote(note);return true;}

