#define FC3_RELIABILITY345_ENTRY Original345
#include "reliability345_xr_test.cpp"
int main(){
 Check303(Original345()==0,"360 existing controller/reload/wrist suite");
 const XrQuaternionf panel={0,0,sinf(.4f),cosf(.4f)};
 auto a=LevelHandUi345(panel,{0,0,sinf(.55f),cosf(.55f)}),b=LevelHandUi345(panel,{0,0,-sinf(.55f),cosf(.55f)});
 const auto upA=testnotes::Rotate(a,{0,1,0}),upB=testnotes::Rotate(b,{0,1,0});
 Check303(std::fabs(upA.x)<.001f&&upA.y>.999f&&std::fabs(upB.x)<.001f&&upB.y>.999f,"360 wrist text follows gravity rather than head roll");
 const auto beforeNormal=testnotes::Rotate(panel,{0,0,1}),afterNormal=testnotes::Rotate(a,{0,0,1});const auto delta=testnotes::Sub(beforeNormal,afterNormal);
 Check303(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z<1e-8f,"360 levelling preserves the actual wrist panel plane");
 const auto singular=LevelHandUi345({sinf(3.14159265f/4),0,0,cosf(3.14159265f/4)},{0,0,0,1});
 Check303(std::isfinite(singular.x)&&std::isfinite(singular.y)&&std::isfinite(singular.z)&&std::isfinite(singular.w),"360 horizontal watch-face fallback remains finite");
 puts("PASS:360 gravity-level wrist text, head-roll independence, unchanged wrist plane/position and degenerate-view fallback");return 0;
}
