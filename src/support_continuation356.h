#pragma once
#include "handle_grab333.h"
#include <cmath>
namespace fc3support356 {
// XR-thread only. Store actual fresh controller-local geometry, never an
// extrapolated world pose. Fallback publications cannot refresh this clock.
struct Cache {
 fc3grab333::Binding last;
 fc3grab333::Binding Choose(const fc3grab333::Binding& current,unsigned entity,unsigned generation,unsigned epoch,unsigned reference,
                           bool latched,bool tracked,float grip,ULONGLONG now){
  const bool family=current.profile>=3&&current.profile<=6;
  const bool identity=family&&current.mode&&entity&&current.entity==entity&&current.generation==generation&&current.epoch==epoch&&current.reference==reference;
  if(!identity){last={};return current;}
  if(current.enabled&&current.support&&now>=current.at&&now-current.at<=120&&!current.physicalReload356){last=current;return current;}
  const bool same=last.enabled&&last.support&&last.profile==current.profile&&last.entity==entity&&last.generation==generation&&last.epoch==epoch&&last.reference==reference;
  if(latched&&tracked&&std::isfinite(grip)&&grip>.35f&&!current.physicalReload356&&same&&now>=last.at&&now-last.at<=120){
   auto carried=last;carried.supportActive=true;carried.supportCarry356=true;carried.busy=current.busy;carried.actionCount=0;carried.reloadNear=false;
   return carried;
  }
  return current;
 }
};
inline Cache& Live(){static Cache cache;return cache;}
}
