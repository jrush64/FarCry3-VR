// [TRIGGER262] Dynamic trigger finger (backlog: "fixed captured finger poses alone are
// not dynamic trigger actuation"). The game's own grip pose has the right index finger
// on the trigger. After every grip pose has been applied, the index chain (bones 7,8,9
// under 2) is blended between that pose (1.0) and a straighter finger, exactly the
// way CurlFingers bends the left hand: each joint's own rotation scaled, no axis guessed.
//   finger not on the trigger sensor and no pull: rest (0.55 of the grip bend, alongside
//   the guard); on the sensor: 0.75 + 0.25 * pull, so a full pull is the game's pose.
// Not applied while the native animation owns the arms, with the machete, the bow's
// string hand, a held throwable, or a CAPTURED right grip (1911, RPG, R700, 1887): those
// finger fits were accepted in headset testing and stay exactly as captured.
double g_triggerFingerRest262=0.55,g_triggerFingerTouch262=0.75;bool g_triggerFingerEnabled262=true;
void TriggerFingerCurl262(float* bones,double amount){
 const FingerChain& chain=kFingers[1];
 Mat4 parentOld=BoneMatrix(bones,chain.parent),parentNew=parentOld;
 for(int k=0;k<3;++k){
  const Mat4 childOld=BoneMatrix(bones,chain.bones[k]);Mat4 inverse={};if(!Invert(parentOld,inverse))break;
  const Mat4 relative=Multiply(inverse,childOld);
  const Mat4 childNew=Multiply(parentNew,PartialTurn(relative,chain.joints[k],amount));
  StoreBone(bones,chain.bones[k],childNew);parentOld=childOld;parentNew=childNew;
 }
}
double TriggerFingerAmount262(const fc3pad::Controllers& c){
 const double pull=(std::max)(0.0,(std::min)(1.0,double(c.rightTrigger)));
 return (c.rightIndexTouch||pull>0.02)?g_triggerFingerTouch262+(1.0-g_triggerFingerTouch262)*pull:g_triggerFingerRest262;
}
void TriggerFinger262(float* bones){
 if(!g_triggerFingerEnabled262)return;const fc3pad::Controllers c=fc3pad::Latest();if(!c.active)return;
 const double amount=TriggerFingerAmount262(c);if(amount>=0.999)return;
 TriggerFingerCurl262(bones,amount);
 static LONG logged=0;static ULONGLONG next=0;const ULONGLONG now=GetTickCount64();
 if(logged<3||now>=next){++logged;next=now+10000;Log("[TRIGGER262] right index finger at %.2f of the grip bend (sensor %d, pull %.2f)",amount,c.rightIndexTouch?1:0,double(c.rightTrigger));}
}
