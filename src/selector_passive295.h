#pragma once
#include "selector290.h"
namespace fc3passive295 {
inline int EquippedMissing(const fc3select290::Inventory& inv,const bool* ready){
 if(!inv.valid||!inv.equipped||!ready)return -1;
 for(int i=0;i<4;++i)if(inv.weapons[i].entity&&inv.weapons[i].entity==inv.equipped&&!ready[i])return i;
 return -1;
}
}
