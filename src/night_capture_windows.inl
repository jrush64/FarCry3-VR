// First-draw captures can belong to a bright load-in. Retain at most six
// genuine draw windows during subsequent gameplay; never reset pending copies.
void NextNightCaptureWindow(LONG frame)
{
 static unsigned windows=1;static LONG next=0;
 if(windows>=6||!g_inGameplay||fc3pad::NativeMenuOpen()||!fc3night::count)return;
 if(!next)next=fc3night::records[0].frame+1200;if(frame<next)return;
 if(g_nightActiveDraws)return;
 for(UINT i=0;i<fc3night::count;++i)if(fc3night::records[i].staging)return;
 for(int stage=0;stage<2;++stage)for(int eye=0;eye<2;++eye)if(!fc3night::claimed[stage][eye])return;
 fc3night::count=0;memset(fc3night::claimed,0,sizeof(fc3night::claimed));++windows;next=frame+1200;
 Log("[NIGHT39] bounded native tone capture window %u/6 armed at frame %ld",windows,frame);
}
