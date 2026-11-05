#pragma once
#include <cmath>
#include <cstdint>

// Shared interaction/release policy. No engine calls, inventory writes or clock
// reads here: rendering/input and the native launch bridge supply explicit data.
namespace fc3manual {
using Id=std::uint64_t;
struct Vec { double x=0,y=0,z=0; };
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,double s){return {a.x*s,a.y*s,a.z*s};}
inline double Dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double Length(Vec a){return std::sqrt(Dot(a,a));}
inline bool Finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
struct Sample {
    Id ms=0,owner=0;unsigned epoch=0;
    Vec world={},relative={}; // release point; same point on WORLD axes with moving body origin removed
    bool tracked=false;
};
struct Release {Id ms=0,owner=0;unsigned epoch=0;Vec origin={},velocity={};};
struct ReleaseMotion152 {double sampledSpeed=0,selectedSpeed=0;Id strokeAge=0;bool assisted=false;};
class History {
    Sample points[16]={};unsigned count=0;
    bool Fit(unsigned first,unsigned last,Vec& worldSlope,Vec& relativeSlope)const{
        if(last<=first||points[last].ms-points[first].ms<12)return false;
        const double n=last-first+1;double mean=0;Vec w={},r={};
        for(unsigned i=first;i<=last;++i){mean+=double(std::int64_t(points[i].ms)-std::int64_t(points[last].ms))*.001;w=w+points[i].world;r=r+points[i].relative;}
        mean/=n;w=w*(1/n);r=r*(1/n);double denominator=0;worldSlope={};relativeSlope={};
        for(unsigned i=first;i<=last;++i){const double t=double(std::int64_t(points[i].ms)-std::int64_t(points[last].ms))*.001-mean;
            denominator+=t*t;worldSlope=worldSlope+(points[i].world-w)*t;relativeSlope=relativeSlope+(points[i].relative-r)*t;}
        if(denominator<1e-8)return false;
        worldSlope=worldSlope*(1/denominator);relativeSlope=relativeSlope*(1/denominator);
        return Finite(worldSlope)&&Finite(relativeSlope)&&Length(worldSlope)<=35&&Length(relativeSlope)<=35;
    }
public:
    void Clear(){count=0;}
    bool Add(const Sample& s){
        if(!s.tracked||!s.ms||!s.owner||!s.epoch||!Finite(s.world)||!Finite(s.relative)) {Clear();return false;}
        if(count){const auto& p=points[count-1];
            if(s.owner!=p.owner||s.epoch!=p.epoch)Clear();
            else if(s.ms<=p.ms){if(s.ms==p.ms)return false;Clear();}
            else if(s.ms-p.ms>120||
                Length(s.relative-p.relative)>.8||Length(s.world-p.world)>1.5)Clear();
        }
        if(count==16){for(unsigned i=1;i<count;++i)points[i-1]=points[i];--count;}
        points[count++]=s;return true;
    }
    unsigned Copy(Sample* out,unsigned capacity)const{const unsigned n=count<capacity?count:capacity;for(unsigned i=0;i<n;++i)out[i]=points[count-n+i];return n;}
    bool AtRelease(Id now,Release& out,double strength=1,ReleaseMotion152* motion=nullptr,double boost=1,bool arcAssist=false)const{
        if(motion)*motion={};
        if(count<3||!std::isfinite(strength)||strength<.25||strength>3||!std::isfinite(boost)||boost<.5||boost>8)return false;
        const auto& last=points[count-1];if(now<last.ms||now-last.ms>40)return false;
        unsigned first=count-1;while(first&&last.ms-points[first-1].ms<=65)--first;
        // PID20248: 42-65 ms Present spacing made three points in65 ms
        // impossible for Molotov. Prefer the short fit; extend only enough
        // to include three tracked samples, bounded to180 ms total.
        while(first&&count-first<3&&last.ms-points[first-1].ms<=180)--first;
        if(count-first<3||last.ms-points[first].ms<16)return false;
        // Fit WORLD positions, including legitimate player translation once.
        // Strength assistance applies to body-relative motion only; it must
        // not multiply locomotion. Both fits share timestamps/rotation samples.
        Vec worldSlope={},relativeSlope={};if(!Fit(first,count-1,worldSlope,relativeSlope))return false;
        // relative is expressed on WORLD axes with the moving camera origin
        // removed by the host. Never combine raw tracking axes with world axes.
        Vec velocity=worldSlope+relativeSlope*(strength-1);
        if(motion)motion->sampledSpeed=motion->selectedSpeed=Length(velocity);
        // The squeeze sensor often reaches release after the hand has slowed
        // or curled down. Retain a coherent stroke for at most100ms, rather
        // than letting that braking tail cancel the throw. Never search an
        // entire wind-up or reuse motion across a tracking discontinuity.
        unsigned stroke=count-1;while(stroke&&last.ms-points[stroke-1].ms<=140)--stroke;
        const Vec travel=last.relative-points[stroke].relative;const double distance=Length(travel);
        Vec selected=relativeSlope;double best=Length(relativeSlope)*1.15;Id age=0;
        if(distance>=.08){
            for(unsigned end=stroke+2;end<count;++end){
                const Id old=last.ms-points[end].ms;if(old>100)continue;
                unsigned begin=end;while(begin>stroke&&points[end].ms-points[begin-1].ms<=65)--begin;
                while(begin>stroke&&end-begin<2&&points[end].ms-points[begin-1].ms<=130)--begin;
                if(end-begin<2||points[end].ms-points[begin].ms<25)continue;
                Vec ws={},rs={};if(!Fit(begin,end,ws,rs))continue;const double speed=Length(rs);
                if(speed<1.2||Dot(rs,travel)<.4*speed*distance)continue;
                unsigned coherent=0;bool reverse=false;
                for(unsigned j=begin+1;j<=end;++j){const Vec delta=points[j].relative-points[j-1].relative;
                    const double len=Length(delta);if(len<.004)continue;
                    const double along=Dot(rs,delta)/(speed*len);if(along>.5)++coherent;else if(along<-.25)reverse=true;}
                const double score=speed*(1-double(old)/250);
                if(reverse||coherent<2||score<=best)continue;
                const Vec candidate=worldSlope-relativeSlope+rs*strength;
                if(!Finite(candidate))continue;
                selected=rs;best=score;age=old;
            }
        }
        // Optional late-overhand assistance. Native world Z is up. Preserve
        // a recent upward/outward hand stroke if the squeeze opens during the
        // downward wrist curl. Horizontal direction comes ONLY from motion.
        // Two samples suffice at slow frame cadence; no old wind-up or idle
        // motion is resurrected, and a vertical downward drop is unchanged.
        if(arcAssist&&relativeSlope.z<-.5){
            const Vec horizontal={travel.x,travel.y,0};const double distance2=Length(horizontal);
            double bestArc=0;
            if(distance2>=.06)for(unsigned end=stroke+1;end<count;++end){
                const Id old=last.ms-points[end].ms;if(old<12||old>100)continue;
                unsigned begin=end-1;while(begin>stroke&&points[end].ms-points[begin-1].ms<=45)--begin;
                Vec ws={},rs={};if(!Fit(begin,end,ws,rs)||rs.z<.75)continue;
                const Vec planar={rs.x,rs.y,0};const double speed=Length(planar);
                if(speed<1||Dot(planar,horizontal)<.65*speed*distance2)continue;
                // An actual reversal after this stroke means the player is
                // winding up or changing direction, rather than curling down.
                bool reversed=false;for(unsigned j=end+1;j<count;++j){const Vec delta=points[j].relative-points[j-1].relative;
                    if(Dot(delta,planar)<-.015*speed){reversed=true;break;}}
                if(reversed)continue;
                const double score=speed*(1-double(old)/200);
                if(score>bestArc){bestArc=score;selected=rs;age=old;}
            }
        }
        velocity=worldSlope-relativeSlope+selected*strength;
        if(!Finite(velocity))return false; // raw fits already rejected tracking discontinuities
        velocity=worldSlope-relativeSlope+selected*(strength*boost);
        const double outputSpeed=Length(velocity);if(outputSpeed>34.9)velocity=velocity*(34.9/outputSpeed);
        if(motion){motion->selectedSpeed=Length(velocity);motion->strokeAge=age;motion->assisted=Length(selected-relativeSlope)>1e-6;}
        out={last.ms,last.owner,last.epoch,last.world,velocity};return true;
    }
};
enum class Item {None,Rock,Frag,Molotov,C4,Mine};
enum class Phase {Idle,Safe,PinHeld,Arming,Armed,Unlit,Igniting,Lit,ReleasePending,AwaitNative};
struct Step {bool accepted=false;double travel=0;};
class Interaction {
    Item item=Item::None;Phase phase=Phase::Idle;Id owner=0;unsigned epoch=0;
    Vec pinStart={},pinAxis={};double pinTravel=0;Id contactSince=0;
public:
    Item Held()const{return item;} Phase State()const{return phase;}
    void Reset(){item=Item::None;phase=Phase::Idle;owner=0;epoch=0;pinTravel=0;contactSince=0;}
    bool Draw(Item selected,Id local,unsigned trackingEpoch,bool available,bool freshPouchGrip){
        if(phase!=Phase::Idle||selected==Item::None||!local||!trackingEpoch||!available||!freshPouchGrip)return false;
        item=selected;owner=local;epoch=trackingEpoch;phase=selected==Item::Molotov?Phase::Unlit:Phase::Safe;return true;
    }
    bool ReturnSafe(){if(phase!=Phase::Safe&&phase!=Phase::PinHeld&&phase!=Phase::Unlit)return false;Reset();return true;}
    bool GrabPin(Vec hand,Vec pin,Vec pullAxis,bool freshGrip){
        if(item!=Item::Frag||phase!=Phase::Safe||!freshGrip||!Finite(hand)||!Finite(pin)||!Finite(pullAxis)||Length(hand-pin)>.035)return false;
        const double n=Length(pullAxis);if(n<.5||n>1.5)return false;
        pinStart=hand;pinAxis=pullAxis*(1/n);pinTravel=0;phase=Phase::PinHeld;return true;
    }
    Step PullPin(Vec hand,bool grip){
        if(item==Item::Frag&&(phase==Phase::Arming||phase==Phase::Armed)&&grip)return {true,1};
        if(phase!=Phase::PinHeld||!Finite(hand))return {};
        if(!grip){phase=Phase::Safe;pinTravel=0;return {};}
        const Vec delta=hand-pinStart;double travel=Dot(delta,pinAxis);
        if(Length(delta-pinAxis*travel)>.06)return {false,pinTravel};
        pinTravel=travel<0?0:travel>.045?.045:travel;
        // The attached hand and pin pose use this bounded fraction, rather than
        // the real controller's unconstrained position (BZ19 end-stop rule).
        if(pinTravel>=.045)phase=Phase::Arming;
        return {true,pinTravel/.045};
    }
    Vec PinHandTarget()const{return pinStart+pinAxis*pinTravel;}
    bool NativeArmAcknowledged(Id local,unsigned trackingEpoch){
        if(item!=Item::Frag||phase!=Phase::Arming||local!=owner||trackingEpoch!=epoch)return false;
        phase=Phase::Armed;return true;
    }
    bool LightWick(Id now,Vec flame,Vec wick,bool lighterLit,bool nativeIgnitionAvailable){
        if(item!=Item::Molotov||phase!=Phase::Unlit||!now)return false;
        if(!Finite(flame)||!Finite(wick)||!lighterLit||!nativeIgnitionAvailable||Length(flame-wick)>.025){contactSince=0;return false;}
        if(!contactSince||now<contactSince){contactSince=now;return false;}
        if(now-contactSince<100)return false;
        // Contact requests ignition. Only native acknowledgement below makes
        // the item lit/throwable; drawing a flame is not an acknowledgement.
        phase=Phase::Igniting;contactSince=0;return true;
    }
    bool NativeIgnitionAcknowledged(Id local,unsigned trackingEpoch){
        if(item!=Item::Molotov||phase!=Phase::Igniting||local!=owner||trackingEpoch!=epoch)return false;
        phase=Phase::Lit;return true;
    }
    bool BeginRelease(const Release& r,bool freshGripRelease){
        if(!freshGripRelease||r.owner!=owner||r.epoch!=epoch||!r.ms||!Finite(r.origin)||!Finite(r.velocity)||Length(r.velocity)>35)return false;
        if(!((item==Item::Rock&&phase==Phase::Safe)||(item==Item::Frag&&phase==Phase::Armed)||(item==Item::Molotov&&phase==Phase::Lit)))return false;
        phase=Phase::ReleasePending;return true;
    }
    bool NativeReleaseRequested(){if(phase!=Phase::ReleasePending)return false;phase=Phase::AwaitNative;return true;}
    bool NativeLaunchAcknowledged(Id local,unsigned trackingEpoch){
        if(phase!=Phase::AwaitNative||local!=owner||trackingEpoch!=epoch)return false;Reset();return true;
    }
    bool CanCancel()const{return phase==Phase::Idle||phase==Phase::Safe||phase==Phase::PinHeld||phase==Phase::Unlit;}
    // Caller handles the native live item/fuse; an armed item never silently
    // becomes a fresh inventory item on a pause, tracking loss or context switch.
};
}
