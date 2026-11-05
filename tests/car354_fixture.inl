void TestNativeCarPose354(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 g_handRimPullCm366[0]=g_handRimPullCm366[1]=g_handRimOutCm366[0]=g_handRimOutCm366[1]=0; // 354 measures the raw native wrist fit; the 365 offset is tested separately
 const Mat4 identity=HandIdentity();g_carHand354={};g_car352={};g_car352.vehicle=456;g_car352.model=0;
 g_car352.owner=1;g_car352.gripping[0]=g_car352.gripping[1]=true;g_car352.gripOffset[0]=0;g_car352.gripOffset[1]=fc3wheel352::Pi;
 g_carBodyToWheel353=identity;g_carSeen352=GetTickCount64();
 for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
 const auto& asset=kCarAssets352[0];
 Mat4 left=identity;const V3 nativePalm=Add(asset.hub,{-asset.radius,.015,0});
 left.m[3]=nativePalm.x+.6781;left.m[7]=nativePalm.y-.0519;left.m[11]=nativePalm.z-1.0387;
 float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,identity);StoreBone(raw,2,left);
 Mat4 fingers[17]={};for(int b=3;b<=19;++b){fingers[b-3]=CarRimTurn354(.005*b);fingers[b-3].m[3]=b*.001;fingers[b-3].m[7]=b*.002;StoreBone(raw,b,Multiply(left,fingers[b-3]));}
 CaptureCarLeft354(456,0,identity,raw);Check(!g_carHand354.hand,"354 right thigh pose is never a grip source");
 for(int i=0;i<5;++i){++g_frame;CaptureCarLeft354(456,1,identity,raw);}
 Check(!g_carHand354.hand,"354 a transient entry pose cannot immediately replace the grip");
 ++g_frame;CaptureCarLeft354(456,1,identity,raw);
 Check(g_carHand354.hand&&g_carHand354.fingers,"354 native left grip captured before override after independent quiet samples");
 Mat4 move={};Check(CarGripMove354(456,1,identity,move),"354 native left wrist retargets to its wheel contact");
 for(int i=0;i<12;++i)Check(std::fabs(move.m[i]-left.m[i])<2e-6,"354 left grip retains actual native wrist orientation and rim offset");
 float controlled[576]={};for(int b=0;b<48;++b)StoreBone(controlled,b,identity);StoreBone(controlled,2,move);
 Check(CarGripFingers354(456,1,controlled),"354 actual native left fingers apply while attached");
 for(int b=3;b<=19;++b){Mat4 expected=Multiply(left,fingers[b-3]);for(int i=0;i<12;++i)Check(std::fabs(controlled[b*12+i]-expected.m[i])<2e-6,"354 left finger articulation matches native grip");}
 Mat4 mirror=identity;mirror.m[0]=-1;Mat4 centered=left;centered.m[3]-=asset.hub.x;centered.m[7]-=asset.hub.y;centered.m[11]-=asset.hub.z;
 Mat4 right=Multiply(mirror,Multiply(centered,mirror));right.m[3]+=asset.hub.x;right.m[7]+=asset.hub.y;right.m[11]+=asset.hub.z;
 Check(CarGripMove354(456,0,identity,move),"354 right wrist uses mirrored left-wheel grip");
 for(int i=0;i<12;++i)Check(std::fabs(move.m[i]-right.m[i])<2e-6,"354 right fit is mirrored around axle and anatomical bind axes");
 StoreBone(controlled,2,move);Check(CarGripFingers354(456,0,controlled),"354 right fingers use native left articulation");
 for(int b=3;b<=19;++b){const Mat4 expected=Multiply(right,Multiply(mirror,Multiply(fingers[b-3],mirror)));for(int i=0;i<12;++i)Check(std::fabs(controlled[b*12+i]-expected.m[i])<2e-6,"354 mirrored native fingers retain right-handed anatomy");}
 auto* upload=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);context->UpdateSubresource(upload,0,nullptr,controlled,0,0);float read[576]={};Read(device,context,upload,read);Compare(controlled,read,576,"354 GPU pose upload preserves native grip");upload->Release();
 const auto oldAngle=g_car352.angle;g_car352.gripping[0]=false;
 Check(!CarGripMove354(456,0,identity,move)&&!CarGripFingers354(456,0,controlled),"354 release immediately returns control to353 free hand");
 Check(g_car352.angle==oldAngle,"354 grip fitting never changes steering state");
 Check(!CarGripMove354(999,1,identity,move),"354 another vehicle cannot reuse the previous car grip");
 const float saved=raw[2*12];raw[2*12]=std::nanf("");g_carHand354={};++g_frame;CaptureCarLeft354(456,1,identity,raw);
 Check(!g_carHand354.hand,"354 invalid native matrices cannot replace the working fallback");raw[2*12]=saved;
 g_carHand354={};g_car352={};g_handRimPullCm366[0]=g_handRimPullCm366[1]=4;g_handRimOutCm366[0]=6;g_handRimOutCm366[1]=1;
 puts("PASS:354 native left steering wrist/finger capture, right anatomical mirror, real rim offsets, GPU upload, entry stability, free-hand release and unchanged steering");
}

