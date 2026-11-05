//338 side-mechanism batch retains337 A2000 behavior and extends its left-owner predicate.
//Conjugation below converts hand/contact frames; mesh vertices use one reflection.
bool A2000LeftCharge336(){
 const auto s=fc3hands303::ReadPresentation314();
 return fc3mirror338::Supported(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&s.enabled&&(s.engaged||s.ownerContext326)&&!s.empty&&s.entity&&s.hand==1&&s.nativeHoldingSide==0;
}
Mat4 A2000ChargeTransform336(const Mat4& native){
 if(!A2000LeftCharge336())return native;
 Mat4 m=native;for(int r=0;r<4;++r)for(int c=0;c<4;++c)if((r==0)!=(c==0))m.m[r*4+c]=-m.m[r*4+c];return m;
}
V3 A2000ChargePoint336(V3 native){if(A2000LeftCharge336())native.x=-native.x;return native;}
