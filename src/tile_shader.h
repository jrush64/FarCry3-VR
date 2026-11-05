#pragma once
// [TILES] lighting tile shaders (VS "main", GS "gs"), shared with tests/tile_clip_test.cpp.
#define FC3_TILE_SHADER_SOURCE     "cbuffer G:register(b0){float4 g[8];};"     "cbuffer T:register(b12){float4 hx;float4 hy;float4 hw;};"     "struct V{float2 c:GAMENDC;float4 q:QUAD;};"     "struct O{float4 p:SV_Position;float4 t:TEXCOORD0;};"     "V main(float4 v:POSITION){V o;o.c=v.xy*g[7].xy+g[7].zw;o.q=float4(g[7].zw,g[7].xy);return o;}"     "float4 E(float2 c){float3 q=float3(c,1);return float4(dot(hx.xyz,q),dot(hy.xyz,q),0,dot(hw.xyz,q));}"     "float Q(float v){return round(saturate((v+1)*0.5)*1023);}"     "O P(float4 e,float4 q,float2 r){O o;float2 n=e.xy/e.w;o.p=float4(n,0,1);o.t=float4(((n-q.xy)/q.zw)*0.5+0.5,r);return o;}"     "[maxvertexcount(9)]void gs(triangle V i[3],inout TriangleStream<O> s){"     "float2 mn=min(min(i[0].c,i[1].c),i[2].c);float2 mx=max(max(i[0].c,i[1].c),i[2].c);"     "int k=0;[unroll]for(int a=0;a<3;a++)k+=abs(i[a].c.x-mx.x)<1e-4?1:0;if(k!=1)return;"     "float2 m=float2(hx.w,hy.w);float2 lo=mn-m;float2 hi=mx+m;"     "if(mn.x<=-0.999)lo.x=-4;if(mn.y<=-0.999)lo.y=-4;if(mx.x>=0.999)hi.x=4;if(mx.y>=0.999)hi.y=4;"     "float2 r=float2(Q(mn.x)*1024+Q(mx.x),Q(mn.y)*1024+Q(mx.y));"     "float4 c[4]={E(lo),E(float2(hi.x,lo.y)),E(hi),E(float2(lo.x,hi.y))};"     "float4 p[5]={c[0],c[0],c[0],c[0],c[0]};int n=0;"     "[unroll]for(int b=0;b<4;b++){float4 A=c[b];float4 B=c[(b+1)%4];float da=A.w-0.01;float db=B.w-0.01;"     "if(da>=0){p[n]=A;n++;}if((da>=0)!=(db>=0)){p[n]=lerp(A,B,da/(da-db));n++;}}"     "float4 q=i[0].q;[unroll]for(int f=1;f<4;f++){if(f+1<n){s.Append(P(p[0],q,r));s.Append(P(p[f],q,r));s.Append(P(p[f+1],q,r));s.RestartStrip();}}}"

// renderfix4 tiles (every triangle through the eye homography, no pixel test): fallback.
#define FC3_TILE_SHADER_SOURCE_SIMPLE \
    "cbuffer G:register(b0){float4 g[8];};" \
    "cbuffer T:register(b12){float4 hx;float4 hy;float4 hw;};" \
    "struct V{float4 e:EYECLIP;float4 q:QUAD;};" \
    "struct O{float4 p:SV_Position;float4 t:TEXCOORD0;};" \
    "V main(float4 v:POSITION){V o;float2 c=v.xy*g[7].xy+g[7].zw;c=c*(abs(c)>0.999?4:1);float3 q=float3(c,1);" \
    "o.e=float4(dot(hx.xyz,q),dot(hy.xyz,q),0,dot(hw.xyz,q));o.q=float4(g[7].zw,g[7].xy);return o;}" \
    "O P(float4 e,float4 q){O o;float2 n=e.xy/e.w;o.p=float4(n,0,1);o.t=float4(((n-q.xy)/q.zw)*0.5+0.5,0,0);return o;}" \
    "[maxvertexcount(6)]void gs(triangle V i[3],inout TriangleStream<O> s){" \
    "float4 p0=0,p1=0,p2=0,p3=0;int n=0;" \
    "[unroll]for(int a=0;a<3;a++){float4 A=i[a].e;float4 B=i[(a+1)%3].e;float da=A.w-0.01;float db=B.w-0.01;" \
    "if(da>=0){if(n==0)p0=A;else if(n==1)p1=A;else if(n==2)p2=A;else p3=A;n++;}" \
    "if((da>=0)!=(db>=0)){float4 C=lerp(A,B,da/(da-db));if(n==0)p0=C;else if(n==1)p1=C;else if(n==2)p2=C;else p3=C;n++;}}" \
    "float4 q=i[0].q;if(n<3)return;" \
    "s.Append(P(p0,q));s.Append(P(p1,q));s.Append(P(p2,q));" \
    "if(n==4){s.RestartStrip();s.Append(P(p0,q));s.Append(P(p2,q));s.Append(P(p3,q));}}"
