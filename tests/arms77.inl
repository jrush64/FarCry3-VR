void TestArmIK77(){
    g_aimActive=false;
    for(int side=0;side<2;++side){
        const double x=side?-.269:.269;
        ArmJoints j={{x,-.24,-.304},{x,-.16,-.114},{x,-.24,.146},true};
        Mat4 hand=HandIdentity(),fore={},upper={};
        const V3 target={x,-.15,.55};
        hand.m[3]=target.x-j.w[0];hand.m[7]=target.y-j.w[1];hand.m[11]=target.z-j.w[2];
        Check(SolveArm(j,hand,side,fore,upper,true,true),"arm77 tracked reach solves");
        const V3 elbow=g_armElbow77[side],upperEnd=Apply(upper,{j.e[0],j.e[1],j.e[2]}),foreStart=Apply(fore,{j.e[0],j.e[1],j.e[2]}),foreEnd=Apply(fore,{j.w[0],j.w[1],j.w[2]});
        const V3 shoulderFar=Apply(upper,{j.s[0],j.s[1],j.s[2]});
        const double farStraight=Dot(Unit(Sub(elbow,shoulderFar)),Unit(Sub(target,elbow)));
        Check(Length(Sub(upperEnd,elbow))<1e-5&&Length(Sub(foreStart,elbow))<1e-5&&Length(Sub(foreEnd,target))<1e-5,"arm77 shoulder elbow wrist form one continuous chain");
        Check(g_armStretch77[side]>1.0&&g_armStretch77[side]<=1.35+1e-6,"arm77 bounded extension active");
        const double slide77=Length(g_armSlide[side]);
        Mat4 oldFore={},oldUpper={};Check(SolveArm(j,hand,side,oldFore,oldUpper,true,false),"legacy arm comparison solves");
        Check(slide77<Length(g_armSlide[side]),"arm77 extension reduces shoulder slide");
        j.e[1]=-.32;
        Check(SolveArm(j,hand,side,fore,upper,true,true),"arm77 animated source elbow alternate solves");
        Check(Length(Sub(g_armElbow77[side],elbow))<1e-5,"arm77 elbow independent of game's animated bend");
        Check(Length(Sub(Apply(fore,{j.w[0],j.w[1],j.w[2]}),target))<1e-5,"arm77 retains exact controller wrist after native elbow moves");
        const V3 nearTarget={x,-.15,.25};hand.m[11]=nearTarget.z-j.w[2];
        Check(SolveArm(j,hand,side,fore,upper,true,true),"arm78 closer hand solves");
        const V3 nearElbow=g_armElbow77[side],shoulderNear=Apply(upper,{j.s[0],j.s[1],j.s[2]});
        const double nearStraight=Dot(Unit(Sub(nearElbow,shoulderNear)),Unit(Sub(nearTarget,nearElbow)));
        Check(nearStraight<farStraight-.1,"arm78 elbow bends visibly when controller moves inward");
        Check(Length(Sub(Apply(fore,{j.w[0],j.w[1],j.w[2]}),nearTarget))<1e-5,"arm78 bent pose keeps tracked wrist exact");
    }
    puts("PASS: arm78 fixed reach bends with inward controller motion, preserves a continuous chain and tracked wrists, ignores native elbow bend on both arms");
}
