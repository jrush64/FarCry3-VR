// Dunia evaluates (worldHigh-cameraHigh)+(worldLow-cameraLow). Rebalance both
// fields, keeping the low remainder below half a high-coordinate float ULP.
// Casting a new absolute world position while retaining the old remainder
// independently quantizes the parent and every child at large coordinates.
bool PistolPreciseTranslation(){return g_weaponMechanicProfile==20&&g_frame-g_weaponMechanicFrame<=2;}
bool WritePistolSplitWorld(float* globals,const Mat4& world){
 if(!globals)return false;
 float high[3],low[3];
 for(int r=0;r<3;++r){
  if(!std::isfinite(globals[r*4+3])||!std::isfinite(world.m[r*4+3]))return false;
  high[r]=static_cast<float>(world.m[r*4+3]);
  low[r]=static_cast<float>(world.m[r*4+3]-double(high[r]));
  if(!std::isfinite(high[r])||!std::isfinite(low[r]))return false;
  for(int c=0;c<3;++c)if(!std::isfinite(world.m[r*4+c]))return false;
 }
 for(int r=0;r<3;++r){for(int c=0;c<3;++c)globals[r*4+c]=static_cast<float>(world.m[r*4+c]);globals[r*4+3]=high[r];globals[12+r]=low[r];}
 return true;
}
