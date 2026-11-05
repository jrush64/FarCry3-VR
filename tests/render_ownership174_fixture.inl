//174: exercise actual collector/budget without a D3D device or GPU readback.
void TestRenderOwnership174()
{
 RenderDrawSample keep[4];memcpy(keep,g_renderDrawSamples,sizeof(keep));
 const LONG keepFrame=g_renderOwnershipFrame174;
 const UINT keepQueries=g_renderOwnershipQueries174,keepOverflow=g_renderOwnershipOverflow174,keepUnexamined=g_renderOwnershipUnexamined174;
 for(auto& s:g_renderDrawSamples)s={};
 g_renderOwnershipFrame174=-1;
 Check(BeginRenderOwnership174(42),"174 first ownership query admitted");
 RenderDrawSample a={};a.frame=42;a.context=1;a.buffer=2;a.target=3;a.depth=4;a.camera.xScale=.4f;
 RetainRenderOwnership174(a);RetainRenderOwnership174(a);
 Check(g_renderDrawSamples[0].frame==42&&g_renderDrawSamples[1].frame!=42,"174 exact ownership duplicate uses one slot");
 auto b=a;b.target=5;RetainRenderOwnership174(b);
 b=a;b.depth=6;RetainRenderOwnership174(b);
 b=a;b.camera.xScale=.8f;RetainRenderOwnership174(b);
 Check(g_renderDrawSamples[1].target==5&&g_renderDrawSamples[2].depth==6&&g_renderDrawSamples[3].camera.xScale==.8f,"174 same CB retains target depth and camera changes");
 b=a;b.viewport.Width=100;Check(!SameRenderOwnership174(a,b),"174 viewport change distinct");RetainRenderOwnership174(b);
 Check(g_renderOwnershipOverflow174==1,"174 fifth distinct observation reports bounded overflow");
 for(int i=1;i<16;++i)Check(BeginRenderOwnership174(42),"174 bounded query admitted");
 Check(!BeginRenderOwnership174(42)&&g_renderOwnershipQueries174==16&&g_renderOwnershipUnexamined174==1,"174 query cap reports unknown remaining draws");
 Check(BeginRenderOwnership174(43)&&g_renderOwnershipQueries174==1&&!g_renderOwnershipOverflow174&&!g_renderOwnershipUnexamined174,"174 new frame resets bounded counters");
 a.frame=43;RetainRenderOwnership174(a);Check(g_renderDrawSamples[0].frame==43,"174 old-frame slot reused");
 memcpy(g_renderDrawSamples,keep,sizeof(keep));g_renderOwnershipFrame174=keepFrame;g_renderOwnershipQueries174=keepQueries;g_renderOwnershipOverflow174=keepOverflow;g_renderOwnershipUnexamined174=keepUnexamined;
}
