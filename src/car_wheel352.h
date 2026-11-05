#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

// Pure wheel interaction in the asset's rest node coordinates. X/Z is the
// rim plane; local Y is the axle. Controller orientation never supplies steer.
//
// 364: a grip is STICKY. Once a hand has taken the rim it stays attached until
// the grip button is let go or the driver context ends (seat exit, menu,
// vehicle change). Depth, radius, tracking hiccups and frame stalls never
// release a hand; they only pause its contribution. Every release stores its
// reason in release363[] and raises released364[] for the caller to log the
// moment it happens.
namespace fc3wheel352 {
constexpr double Pi=3.14159265358979323846;
constexpr double kGrabDepth364=.20;      // |y| from the rim plane accepted for a new grip
constexpr double kGrabRadial364=.12;     // |r-radius| accepted for a new grip
constexpr double kHubDeadZone364=.06;    // r below this: hold the angle, no steering delta
constexpr double kGlitchTurn364=2.5;     // a per-update polar jump beyond this is a tracking glitch, ignored
enum Release364:unsigned{kTracking=1,kGrip=2,kDepth=3,kRadius=4,kJump=5,kContext=6,kStale=7};
struct Hand { double x=0,y=0,z=0; float grip=0; bool tracked=false; };
struct Input {
    std::uint64_t vehicle=0, now=0;
    int model=-1;
    bool allowed=false;
    double radius=.2, nativeAngle=0, fullLock=Pi/2, smoothing363=0;
    Hand hand[2]; // physical right, physical left
};
struct Wheel {
    std::uint64_t vehicle=0,last=0;
    int model=-1, owner=-1;
    double angle=0,target363=0; double previousPolar[2]={},gripOffset[2]={};
    unsigned release363[2]={};
    unsigned released364[2]={}; // reason of a release raised by the latest Update, 0 otherwise
    bool grabbed364[2]={};      // a grip was taken by the latest Update
    bool gripping[2]={},waitRelease[2]={false,false},paused364[2]={};
    void Drop(unsigned reason=kContext){for(int s=0;s<2;++s)if(gripping[s]){release363[s]=reason;released364[s]=reason;}owner=-1;gripping[0]=gripping[1]=false;paused364[0]=paused364[1]=false;}
    bool Update(const Input& in) {
        const auto previous363=last;
        released364[0]=released364[1]=0;grabbed364[0]=grabbed364[1]=false;
        const bool changed=vehicle!=in.vehicle||model!=in.model;
        if(changed){Drop();vehicle=in.vehicle;model=in.model;angle=target363=in.nativeAngle;}
        if(!in.allowed||!in.vehicle||in.model<0||!std::isfinite(in.nativeAngle)||
           !std::isfinite(in.radius)||in.radius<.1||in.radius>.35||
           !std::isfinite(in.fullLock)||in.fullLock<.1||in.fullLock>Pi*2){
            Drop(kContext);waitRelease[0]=waitRelease[1]=true;last=in.now;return false;
        }
        last=in.now;
        for(int side=0;side<2;++side)if(in.hand[side].grip<.3f)waitRelease[side]=false;
        auto valid=[](const Hand& h){return h.tracked&&std::isfinite(h.x)&&std::isfinite(h.y)&&std::isfinite(h.z)&&std::isfinite(h.grip);};
        double sum=0;int count=0;
        for(int side=0;side<2;++side)if(gripping[side]){
            const Hand& h=in.hand[side];
            if(std::isfinite(h.grip)&&h.grip<.35f){
                // The only hand-driven release: the grip button let go.
                release363[side]=released364[side]=kGrip;gripping[side]=false;paused364[side]=false;continue;
            }
            if(!valid(h)){
                // Tracking hiccup or stale pose: keep the hand on the rim, no delta.
                // The next valid sample re-anchors so the gap cannot snap the wheel.
                paused364[side]=true;continue;
            }
            const double r=std::hypot(h.x,h.z),polar=std::atan2(h.z,h.x);
            if(r<kHubDeadZone364){paused364[side]=true;continue;} // hand over the hub: direction undefined, hold
            if(paused364[side]){paused364[side]=false;previousPolar[side]=polar;continue;}
            const double turn=std::remainder(polar-previousPolar[side],2*Pi);
            previousPolar[side]=polar;
            if(std::fabs(turn)>kGlitchTurn364)continue; // one-sample glitch: ignore the delta, stay attached
            sum+=turn;++count;
        }
        // With two attached hands use their common circular movement. A new
        // second grip captures its contact without adding a steering delta.
        if(count){target363=(std::max)(-in.fullLock,(std::min)(in.fullLock,target363+sum/count));const double dt=previous363&&in.now>=previous363?(in.now-previous363)*.001:0;const double alpha=in.smoothing363>0&&std::isfinite(in.smoothing363)?-std::expm1(-dt/in.smoothing363):1;angle+=alpha*(target363-angle);}
        else if(!gripping[0]&&!gripping[1])angle=target363=in.nativeAngle;
        for(int side=0;side<2;++side)if(!gripping[side]){
            const Hand& h=in.hand[side];
            if(!waitRelease[side]&&valid(h)&&h.grip>.7f&&std::fabs(h.y)<kGrabDepth364&&
               std::fabs(std::hypot(h.x,h.z)-in.radius)<kGrabRadial364){
                gripping[side]=true;grabbed364[side]=true;paused364[side]=false;previousPolar[side]=std::atan2(h.z,h.x);gripOffset[side]=previousPolar[side]-angle;
            }
        }
        if(owner<0||!gripping[owner])owner=gripping[0]?0:(gripping[1]?1:-1);
        return owner>=0;
    }
    double Demand(double fullLock)const{return owner<0?0:(std::max)(-1.,(std::min)(1.,-angle/fullLock));}
};
inline const char* ReleaseName364(unsigned reason){
    switch(reason){case kTracking:return "tracking";case kGrip:return "grip let go";case kDepth:return "depth";case kRadius:return "radius";
    case kJump:return "jump";case kContext:return "context";case kStale:return "stale";default:return "none";}
}
}
