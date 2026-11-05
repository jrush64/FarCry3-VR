#pragma once
#include <cmath>
#include <cstdint>
namespace fc3pouch361 {
struct State {
 bool armed=false,down=false;unsigned epoch=0;std::uint64_t at=0;
 bool Step(bool allowed,bool tracked,float grip,float distance,unsigned currentEpoch,std::uint64_t now){
  const bool stale=at&&(now<at||now-at>120);at=now;
  if(!allowed||!tracked||!std::isfinite(grip)||!std::isfinite(distance)||stale||epoch!=currentEpoch){epoch=currentEpoch;armed=false;down=grip>.35f;return false;}
  if(grip<.25f){armed=true;down=false;return false;}
  const bool press=grip>.7f&&!down;if(grip>.7f)down=true;
  if(!press)return false;const bool draw=armed&&distance<=.12f;armed=false;return draw;
 }
};
}
