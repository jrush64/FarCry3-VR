// Unmodified prior362 placement formulas for the regression witness.
bool LegacyCarGripMove363(unsigned long long vehicle,int side,const Mat4& neutralView,Mat4& move){
    const auto& p=g_carHand354;
    if(!CarAttached354(vehicle,side)||!p.hand||p.vehicle!=vehicle||p.epoch!=g_armPoseEpoch||p.model!=g_car352.model)return false;
    Mat4 wheelToBody={},inverseNeutral={};if(!Invert(g_carBodyToWheel353,wheelToBody)||!Invert(neutralView,inverseNeutral))return false;
    Mat4 canonical=p.canonicalLeft;
    if(side==0){ // reflect wheel X and bind-pose X, then return to the same rim-angle reference
        Mat4 mirror={};mirror.m[0]=-1;mirror.m[5]=mirror.m[10]=mirror.m[15]=1;
        canonical=Multiply(CarRimTurn354(-fc3wheel352::Pi),Multiply(mirror,Multiply(canonical,mirror)));
    }
    const double polar=g_car352.angle+g_car352.gripOffset[side];
    Mat4 desired=Multiply(CarRimTurn354(polar),canonical);
    const auto& hub=kCarAssets352[p.model].hub;desired.m[3]+=hub.x;desired.m[7]+=hub.y;desired.m[11]+=hub.z;
    const Mat4 view=Multiply(CarHeadView352(),Multiply(wheelToBody,desired));
    move=Multiply(view,inverseNeutral);return true;
}
bool LegacyBarGripMove363(unsigned long long id,int side,const Mat4& neutral,Mat4& move){
 const auto& p=g_barHand355[side];if(!BarAttached355(id,side)||!p.valid||p.id!=id)return false;Mat4 inverse={},invNeutral={};
 if(!Invert(g_bodyToBar355,inverse)||!Invert(neutral,invNeutral))return false;
 move=Multiply(Multiply(CarHeadView352(),Multiply(inverse,Multiply(BarTurn355(g_bars355.angle),p.hand))),invNeutral);return true;
}
