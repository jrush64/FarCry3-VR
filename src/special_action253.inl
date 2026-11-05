// Visual guidance only. Existing physical mechanisms own ammo and readiness.
int SpecialActionRequired253(){
 if(FlamerActive()&&FlamerFrameReady()){
  const auto& f=g_flamer;
  const bool unfinished=FlamerLoaded()==0||f.lever>.02||f.leverDown||f.leverGrab||FlamerClipPlaying()||!f.seated||f.carrying||f.pending;
  if(!unfinished)return 0;
  return f.leverDown&&(!f.replaced||f.carrying||!f.seated)?9:8;
 }
 if(FlareActive()&&FlareFrameReady()){
  const auto& f=g_flare;
  if(f.pending)return 0;
  if(f.open&&(f.chamber!=1||f.carrying))return 11;
  return f.open||f.chamber!=1||FlareLoadedRounds()==0?10:0;
 }
 if(Gm94Active()&&Gm94FrameReady()){
  const auto& g=g_gm94;if(g.pending||g.carrying)return 0;
  if(g.coverOpen||g.coverGrab||g_gm94Clip.kind)return 12;
  if(g.spent||g.needsPump||g.pumping)return 13;
  if(Gm94Loaded()==0)return 12;
  return !g.chambered?13:0;
 }
 return 0;
}
int SpecialActionDraw253(const DrawCall& call){
 if(g_weaponMechanicProfile==15){
  if((call.b==3630&&call.a==8004)||(call.b==20838&&call.a==13380))return 8;
  if(call.b==13590&&call.a==7248)return 9;
 }
 if(g_weaponMechanicProfile==16){
  if(call.b==0&&call.a==1590)return 10;
  if((call.b==8616&&call.a==138)||(call.b==8754&&call.a==330))return 11;
 }
 if(g_weaponMechanicProfile==18){
  if((call.b==14820&&call.a==1638)||(call.b==16458&&call.a==3354)||(call.b==0&&call.a==48))return 12;
  if((call.b==9900&&call.a==2952)||(call.b==12852&&call.a==456)||(call.b==13308&&call.a==1512))return 13;
 }
 return 0;
}
bool SpecialActionGeometry253(int action,const fc3weapon::Geometry& d){
 if(d.format!=57)return false;
 if(action==8||action==9)return d.vertexBytes==349160&&d.indexBytes==68436&&d.stride==40&&d.offset==0;
 if(action==10||action==11)return d.vertexBytes==91584&&d.indexBytes==18168&&d.stride==32&&d.offset==0;
 if(action==12||action==13)return d.vertexBytes==243072&&d.indexBytes==40776&&((d.stride==32&&d.offset==640)||(action==12&&d.stride==20&&d.offset==0));
 return false;
}
