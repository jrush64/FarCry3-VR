#include "../src/locker_probe329.h"
#include <cstdio>
#include <cstdlib>
void Check329(bool good,const char* name){if(!good){printf("FAIL:%s\n",name);exit(1);}printf("PASS:%s\n",name);}
extern bool PublishOther329(fc3locker329::Native&);
int main(){
 using namespace fc3locker329;Sampler sample;Check329(sample.Due(1000)&&!sample.Due(1099)&&sample.Due(1100)&&sample.Due(800),"329100ms native sampling bound and clock reset");
 Budget budget;Check329(budget.Take(2)&&budget.Take(2)&&!budget.Take(2)&&!budget.Take(2),"329 log budget remains bounded");
 Native a;a.inv.valid=true;a.inv.owner=1;a.inv.equipped=100;a.inv.at=1000;
 for(int i=0;i<4;++i){a.inv.weapons[i].entity=100+i;a.inv.weapons[i].id=200+i;a.inv.weapons[i].wrapper=300+i;strcpy_s(a.inv.weapons[i].name,"old");a.weapon[i]=400+i;a.properties[i]=500+i;}
 const auto before=a.inv;Check329(Publish(a)&&a.revision==1,"329 initial four-slot native observation");auto b=a;b.inv.at=1100;Check329(!Publish(b)&&b.revision==1&&Read().inv.at==1100,"329 heartbeat updates freshness without repetitive events");
 b=a;strcpy_s(b.inv.weapons[2].name,"DesertEagle");Check329(Publish(b)&&b.revision==2,"329 weapon name change detected even with reused entity and ID");
 b.lease=false;for(int i=0;i<4;++i){b.inv.weapons[i].entity+=1000;b.inv.weapons[i].id+=1000;b.properties[i]+=1000;strcpy_s(b.inv.weapons[i].name,"new");}
 Check329(Publish(b)&&Read().inv.weapons[3].entity==1103&&!Read().lease,"329 four replacements observed while interaction lease is absent");
 Check329(!memcmp(&a.inv,&before,sizeof(before)),"329 observation does not mutate caller inventory or issue commands");
 auto c=b;c.properties[1]++;Check329(!Same(b,c),"329 resident native properties change visible without new entity");c=b;c.inv.equipped++;Check329(!Same(b,c),"329 manual equip transition observed separately from model capture");
 Visual v;v.rawReady=15;v.ready=1;v.nativeRevision=b.revision;auto w=v;Check329(v==w,"329 visual snapshot stable without changes");w.ready=15;Check329(!(v==w),"329 raw cached models distinguished from usable models");w=v;w.nativeRevision++;Check329(!(v==w),"329 new native loadout reports even when displayed cache stays stale");w=v;w.rawId[3]++;Check329(!(v==w),"329 cached identity changes detected");
 auto other=Read();other.inv.owner+=100;Check329(PublishOther329(other)&&Read().inv.owner==other.inv.owner,"329 native/render translation units share one observation snapshot");
 return 0;
}
