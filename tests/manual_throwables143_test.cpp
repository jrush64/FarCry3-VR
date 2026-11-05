#include "../src/manual_throwables143.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace fc3manual;
void Check(bool ok,const char* name){if(!ok){std::printf("FAIL: %s\n",name);std::exit(1);}}
bool Near(double a,double b){return std::fabs(a-b)<1e-6;}
int main(){
    {History boosted175;Release output175;
     for(unsigned i=0;i<5;++i)boosted175.Add({12000+i*20,9,3,{i*.03,0,0},{i*.01,0,0},true});
     Check(boosted175.AtRelease(12080,output175,1,nullptr,4)&&Near(output175.velocity.x,3),"175 previous boost retains moving-player translation once");
     Check(boosted175.AtRelease(12080,output175,1,nullptr,8)&&Near(output175.velocity.x,5),"175 extra Molotov headroom changes measured hand impulse without multiplying locomotion");
     Check(!boosted175.AtRelease(12080,output175,1,nullptr,8.1),"175 excessive boost rejected");
     boosted175.Clear();for(unsigned i=0;i<5;++i)boosted175.Add({13000+i*20,9,3,{1,2,3},{0,0,0},true});
     Check(boosted175.AtRelease(13080,output175,1,nullptr,8)&&Length(output175.velocity)<1e-6,"175 maximum boost does not invent a lob for a stationary drop");}
    History h;Release r;
    for(Id ms=1000;ms<=1060;ms+=10){double t=(ms-1000)*.001;
        h.Add({ms,9,2,{400+3*t,-200+2*t,50+4*t},{2*t,0,4*t},true});}
    Check(h.AtRelease(1061,r)&&Near(r.origin.x,400.18)&&Near(r.velocity.x,3)&&Near(r.velocity.y,2)&&Near(r.velocity.z,4),"world release fit, moving player included once, latest actual origin");
    Check(h.AtRelease(1061,r,2)&&Near(r.velocity.x,5)&&Near(r.velocity.y,2)&&Near(r.velocity.z,8),"assistance scales hand motion without multiplying locomotion");
    Check(!h.AtRelease(1101,r),"stale release rejected");
    h.Add({1070,9,3,{400,0,0},{0,0,0},true});Check(!h.AtRelease(1071,r),"recenter epoch resets velocity history");
    h.Add({1080,9,3,{400,0,0},{0,0,0},false});Check(!h.AtRelease(1081,r),"tracking loss clears release history");
    for(Id ms=1100;ms<=1130;ms+=10)h.Add({ms,9,4,{0,0,0},{0,0,0},true});
    Check(h.AtRelease(1130,r)&&Near(Length(r.velocity),0),"gentle stationary release is a drop, no artificial forward speed");
    h.Add({1140,9,4,{30,0,0},{30,0,0},true});Check(!h.AtRelease(1140,r),"teleport not interpreted as throw");
    for(Id ms=1200;ms<=1240;ms+=10)h.Add({ms,9,5,{0,double(ms-1200)*.004,0},{0,double(ms-1200)*.004,0},true});
    Check(h.AtRelease(1240,r)&&Near(r.velocity.x,0)&&Near(r.velocity.y,4),"sideways throw follows motion, no camera-forward assumption");
    Check(!h.AtRelease(1240,r,NAN),"invalid assistance rejected");
    for(unsigned spacing:{42u,50u,65u}){
        h.Clear();for(Id ms=3000;ms<=3000+spacing*4;ms+=spacing){const double t=(ms-3000)*.001;h.Add({ms,9,5,{t*2,0,0},{t*2,0,0},true});}
        Check(h.AtRelease(3000+spacing*4,r)&&Near(r.velocity.x,2),"recorded42-65ms frame cadence still yields physical release");
    }
    h.Add({3400,9,5,{.8,0,0},{.8,0,0},false});Check(!h.AtRelease(3400,r),"extended cadence still rejects explicit tracking loss");
    const Release interactionRelease=r;ReleaseMotion152 motion;
    h.Clear();for(Id t=0;t<=160;t+=20){const double x=t<=100?t*.006:.6+(t-100)*.0005;h.Add({4000+t,9,6,{x,0,0},{x,0,0},true});}
    Check(h.AtRelease(4160,r,1,&motion)&&motion.assisted&&r.velocity.x>5.5&&motion.strokeAge<=100&&Near(r.origin.x,.63),"late grip opening retains recent coherent throw speed with latest release origin");
    h.Clear();for(Id t=0;t<=160;t+=20){const double y=t<=100?-double(t)*.006:-.6-(t-100)*.0005;h.Add({5000+t,9,6,{t*.002,y,0},{0,y,0},true});}
    Check(h.AtRelease(5160,r,2,&motion)&&motion.assisted&&Near(r.velocity.x,2)&&r.velocity.y<-11&&Near(r.velocity.z,0),"assisted sideways stroke scales only hand motion and preserves locomotion exactly once");
    for(Id t=180;t<=340;t+=20)h.Add({5000+t,9,6,{t*.002,-.63,0},{0,-.63,0},true});
    Check(h.AtRelease(5340,r,1,&motion)&&!motion.assisted&&Near(r.velocity.x,2)&&Near(r.velocity.y,0),"deliberate stop then release never reuses an expired throw stroke");
    h.Clear();for(Id t=0;t<=200;t+=20){const double x=t<=80?-double(t)*.008:-.64+(t-80)*.006;h.Add({6000+t,9,6,{x,0,0},{x,0,0},true});}
    Check(h.AtRelease(6200,r,1,&motion)&&r.velocity.x>5.5,"backward wind-up does not replace the following forward stroke");
    h.Add({6210,9,6,{0,0,0},{0,0,0},false});Check(!h.AtRelease(6210,r,1,&motion),"timing assistance cannot reuse throw speed after tracking loss");
    std::puts("PASS:152 late grip release preserves coherent recent stroke, actual release point, sideways direction, locomotion and stationary/stop/tracking safety.");
    h.Clear();for(Id t=0;t<=140;t+=20){const double x=t*.004,z=t<=80?t*.006:.48-(t-80)*.012;h.Add({7000+t,9,7,{x,0,z},{x,0,z},true});}
    Check(h.AtRelease(7140,r,1,&motion,1,false)&&r.velocity.z<-10,"late overhand grip opening reproduces downward raw launch");
    Check(h.AtRelease(7140,r,1,&motion,1.5,true)&&motion.assisted&&r.velocity.x>5.9&&r.velocity.z>8.9,"overhand assist retains recent outward/upward motion and boost scales its distance");
    h.Clear();for(Id t=0;t<=140;t+=20){const double z=t<=80?t*.006:.48-(t-80)*.012;h.Add({8000+t,9,7,{0,0,z},{0,0,z},true});}
    Check(h.AtRelease(8140,r,1,&motion,1.5,true)&&r.velocity.z<-15,"intentional downward vertical motion is never changed into a forward/upward lob");
    h.Clear();for(Id t=0;t<=60;t+=20)h.Add({9000+t,9,7,{t*.002,0,0},{0,0,0},true});
    Check(h.AtRelease(9060,r,1,&motion,4,true)&&Near(r.velocity.x,2)&&Near(r.velocity.z,0),"maximum boost cannot multiply locomotion or invent a stationary throw");
    h.Clear();for(Id t=0;t<=60;t+=20)h.Add({10000+t,9,7,{t*.012,0,0},{t*.012,0,0},true});
    Check(h.AtRelease(10060,r,1,&motion,4,true)&&r.velocity.x>34&&r.velocity.x<35,"strong boosted throw stays physical instead of being rejected into a drop");
    Check(!h.AtRelease(10060,r,1,&motion,NAN,true),"invalid boost rejected");
    std::puts("PASS:156 late overhand curl, motion-only boost, vertical drop, locomotion isolation and strong-throw bound.");
    r=interactionRelease;
    Interaction i;
    Check(!i.Draw(Item::Frag,9,5,true,false)&&!i.Draw(Item::Frag,9,5,false,true),"draw requires available item and fresh pouch grip");
    Check(i.Draw(Item::Frag,9,5,true,true)&&i.ReturnSafe()&&i.State()==Phase::Idle,"unarmed grenade can be returned without native consumption");
    Check(i.Draw(Item::Frag,9,5,true,true)&&!i.BeginRelease(r,true),"unarmed grenade cannot enter native launch");
    Check(!i.GrabPin({.2,0,0},{0,0,0},{1,0,0},true),"pin requires actual nearby offhand contact");
    Check(i.GrabPin({0,0,0},{0,0,0},{1,0,0},true),"pin contact acquired");
    auto p=i.PullPin({.02,0,0},true);Check(p.accepted&&Near(p.travel,.02/.045)&&i.State()==Phase::PinHeld,"partial pin pull uses bounded travel");
    Check(!i.PullPin({.02,.2,0},true).accepted,"sideways off-axis pin motion rejected");
    i.PullPin({.02,0,0},false);Check(i.State()==Phase::Safe,"release during partial pull restores safe state");
    i.GrabPin({0,0,0},{0,0,0},{1,0,0},true);p=i.PullPin({.5,0,0},true);
    Check(p.accepted&&Near(p.travel,1)&&i.State()==Phase::Arming&&!i.CanCancel()&&!i.ReturnSafe(),"pin hand stops at full travel, native arming cannot silently return/reset");
    Check(Near(i.PullPin({1,0,0},true).travel,1)&&Near(i.PinHandTarget().x,.045),"continued physical pulling cannot move attached pose past end stop");
    Check(!i.BeginRelease(r,true)&&!i.NativeArmAcknowledged(10,5)&&i.NativeArmAcknowledged(9,5),"pin pose alone does not prove native grenade arming");
    Check(i.BeginRelease(r,true)&&i.NativeReleaseRequested()&&!i.NativeReleaseRequested(),"one native release request per grip release");
    Check(!i.NativeLaunchAcknowledged(10,5)&&i.NativeLaunchAcknowledged(9,5)&&i.State()==Phase::Idle,"only matching owner/epoch launch consumes local interaction");
    Check(i.Draw(Item::Molotov,9,5,true,true)&&!i.BeginRelease(r,true),"unlit Molotov cannot use lit-projectile path");
    Check(!i.LightWick(2000,{0,0,0},{0,0,0},true,false)&&!i.LightWick(2200,{0,0,0},{0,0,0},true,false),"particles alone cannot claim native ignition");
    Check(!i.LightWick(2300,{0,0,0},{0,0,0},true,true),"lighter wick contact requires dwell");
    Check(!i.LightWick(2350,{0,0,0},{.1,0,0},true,true),"broken contact resets dwell");
    Check(!i.LightWick(2400,{0,0,0},{.01,0,0},true,true)&&i.LightWick(2500,{0,0,0},{.01,0,0},true,true),"lit lighter ignites only after continuous wick contact and native availability");
    Check(!i.BeginRelease(r,true)&&!i.NativeIgnitionAcknowledged(9,6)&&i.NativeIgnitionAcknowledged(9,5),"visible wick contact requires matching native ignition acknowledgement before lit release");
    Check(!i.LightWick(2600,{0,0,0},{0,0,0},true,true)&&i.BeginRelease(r,true),"lit state fires once and permits physical release");
    std::puts("PASS: physical release velocity/origin, locomotion, drops, sideways throws, recenter/tracking discontinuities, safe stow, bounded pin/end stop, one-shot release, and lighter-to-wick state policy. Engine ignition and headset behavior not simulated.");
}
