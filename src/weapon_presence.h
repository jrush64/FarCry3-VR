#pragma once
// Generated from the existing FC3 offline asset corpus. Full buffer/stream/index
// identity is required; only a main weapon mesh qualifies for presence. These
// profiles identify geometry, not acceptance of motion behavior on every weapon.
namespace fc3weapon {
struct Geometry { unsigned vertexBytes,indexBytes,stride,offset,start,count,format; };
constexpr Geometry kProfiles[]={
    {217840,47196,32,400,42,10899,57}, // 1887.xbg
    {106448,20736,32,240,5958,3468,57}, // 6p9.xbg
    {368728,62190,32,9560,8742,13419,57}, // 700sniperrifle.xbg
    {257896,47478,40,336,486,4071,57}, // 870shotgun.xbg
    {257896,47478,40,336,4557,5076,57}, // 870shotgun.xbg
    {257896,47478,40,336,9633,6795,57}, // 870shotgun.xbg
    {257896,47478,40,336,16428,7311,57}, // 870shotgun.xbg
    {214976,45228,32,0,0,8754,57}, // agram2000.xbg
    {250208,51150,32,0,1056,17421,57}, // ak74m.xbg
    {259616,48300,32,0,4875,7521,57}, // bison.xbg
    {230760,44856,40,0,0,9852,57}, // bow2.xbg
    {45320,10524,40,0,0,5262,57}, // chineseknife.xbg
    {202816,39126,32,320,2004,8148,57}, // desert_eagle.xbg
    {325680,46740,40,0,7950,15420,57}, // dshk.xbg
    {216704,39378,32,0,4728,6528,57}, // famas.xbg
    {349160,68436,40,0,20838,13380,57}, // flamethrower.xbg
    {91584,18168,32,0,3708,4002,57}, // flare_gun.xbg
    {210720,38826,32,0,9918,5301,57}, // galilace52.xbg
    {243072,40776,32,640,2544,7356,57}, // gm94.xbg
    {201920,38760,40,0,720,18660,57}, // tanto.xbg
    {177904,32682,32,240,294,4470,57}, // kimber.xbg
    {311800,46470,40,0,3615,10623,57}, // kriss.xbg
    {309952,59472,32,0,2388,10752,57}, // minimi.xbg
    {388232,85386,32,10760,16437,8025,57}, // m93.xbg
    {71720,12120,40,0,0,2910,57}, // machete.xbg
    {291640,49818,40,0,11028,13881,57}, // mk16.xbg
    {112928,22014,32,0,0,6495,57}, // mk23.xbg
    {43360,11358,40,0,2799,2880,57}, // molotov.xbg
    {306360,55536,40,0,12234,15534,57}, // mp5.xbg
    {29632,8268,32,0,1590,1776,57}, // npc_knife.xbg
    {31872,9684,32,0,1770,2304,57}, // npc_knife_2.xbg
    {341536,65556,32,0,2853,13368,57}, // p416.xbg
    {201024,43572,32,0,0,21786,57}, // rpg7.xbg
    {59392,18300,32,0,4344,4806,57}, // shiv.xbg
    {127600,30018,32,240,4908,7401,57}, // spas12.xbg
    {268464,59526,32,10160,13710,11358,57}, // svd.xbg
    {247392,45756,32,0,2406,12300,57}, // u100.xbg
    {41824,13176,32,0,0,6588,57}, // ulc_tribalknife.xbg
    {119488,26364,32,0,0,6924,57}, // ulc_type10flaregun.xbg
    {157984,35322,32,0,3072,10347,57}, // vz61.xbg
    {213320,34200,40,0,4824,12276,57}, // welding_torch.xbg
    {304128,52560,32,0,10782,13122,57}, // sg551.xbg (STG-90 main Weapon metal draw, offline catalog)
    {186104,31002,40,224,12,9432,57}, // stealthhunter.xbg (.44 Magnum revolver main skinned draw, capture PID3384)
    {636120,83850,40,0,14397,23700,57}, // pkm.xbg (skinned main draw, capture weapons6-115332; motion controls need a profile)

};
constexpr const char* kProfileNames[]={
    "1887.xbg",
    "6p9.xbg",
    "700sniperrifle.xbg",
    "870shotgun.xbg",
    "870shotgun.xbg",
    "870shotgun.xbg",
    "870shotgun.xbg",
    "agram2000.xbg",
    "ak74m.xbg",
    "bison.xbg",
    "bow2.xbg",
    "chineseknife.xbg",
    "desert_eagle.xbg",
    "dshk.xbg",
    "famas.xbg",
    "flamethrower.xbg",
    "flare_gun.xbg",
    "galilace52.xbg",
    "gm94.xbg",
    "tanto.xbg",
    "kimber.xbg",
    "kriss.xbg",
    "minimi.xbg",
    "m93.xbg",
    "machete.xbg",
    "mk16.xbg",
    "mk23.xbg",
    "molotov.xbg",
    "mp5.xbg",
    "npc_knife.xbg",
    "npc_knife_2.xbg",
    "p416.xbg",
    "rpg7.xbg",
    "shiv.xbg",
    "spas12.xbg",
    "svd.xbg",
    "u100.xbg",
    "ulc_tribalknife.xbg",
    "ulc_type10flaregun.xbg",
    "vz61.xbg",
    "welding_torch.xbg",
    "sg551.xbg",
    "stealthhunter.xbg",
    "pkm.xbg",
};
static_assert(sizeof(kProfileNames)/sizeof(*kProfileNames)==sizeof(kProfiles)/sizeof(*kProfiles), "profile name count");
inline int ProfileIndex(const Geometry& draw)
{
    for(unsigned i=0;i<sizeof(kProfiles)/sizeof(*kProfiles);++i)
    {
        const auto& p=kProfiles[i];
        if(draw.vertexBytes==p.vertexBytes&&draw.indexBytes==p.indexBytes&&draw.stride==p.stride&&
           draw.offset==p.offset&&draw.format==p.format&&draw.start==p.start&&draw.count==p.count)return static_cast<int>(i);
    }
    return -1;
}
struct ModelGeometry { unsigned vertexBytes,indexBytes,format; };
constexpr ModelGeometry kModels[]={
    {217840,47196,57}, // 1887.xbg
    {106448,20736,57}, // 6p9.xbg
    {368728,62190,57}, // 700sniperrifle.xbg
    {7680,1488,57}, // r700_highpowered.xbg
    {14976,2712,57}, // r700_illuminated.xbg
    {257896,47478,57}, // 870shotgun.xbg
    {98144,21168,57}, // acog.xbg
    {120816,24948,57}, // optical.xbg
    {48520,7032,57}, // reflex_a.xbg
    {67448,11280,57}, // reflex_b.xbg
    {10560,2160,57}, // silencer_handgun.xbg
    {214976,45228,57}, // agram2000.xbg
    {250208,51150,57}, // ak74m.xbg
    {2720,492,57}, // cleaningrod.xbg
    {37504,9144,57}, // ammmobox_shotgun.xbg
    {259616,48300,57}, // bison.xbg
    {19776,4704,57}, // arrow.xbg
    {41376,6924,57}, // arrow_expl.xbg
    {31296,8100,57}, // arrow_flam.xbg
    {230760,44856,57}, // bow2.xbg
    {64600,11400,57}, // bow_sight_crosshair.xbg
    {19904,3684,57}, // bow_sight_pegs.xbg
    {54456,9066,57}, // bow_sight_relfex.xbg
    {45320,10524,57}, // chineseknife.xbg
    {202816,39126,57}, // desert_eagle.xbg
    {325680,46740,57}, // dshk.xbg
    {216704,39378,57}, // famas.xbg
    {349160,68436,57}, // flamethrower.xbg
    {1600,264,57}, // flare.xbg
    {91584,18168,57}, // flare_gun.xbg
    {210720,38826,57}, // galilace52.xbg
    {243072,40776,57}, // gm94.xbg
    {37888,9276,57}, // frag_grenade.xbg
    {143132,32760,57}, // ied_c4_bomb.xbg
    {49632,12726,57}, // ied_c4_remote.xbg
    {201920,38760,57}, // tanto.xbg
    {177904,32682,57}, // kimber.xbg
    {311800,46470,57}, // kriss.xbg
    {309952,59472,57}, // minimi.xbg
    {388232,85386,57}, // m93.xbg
    {7744,1680,57}, // m93_highpowered.xbg
    {16800,3312,57}, // m93_illuminated.xbg
    {12544,2340,57}, // m93_muzzlebreak.xbg
    {12928,2340,57}, // m93_muzzlebreak_separate.xbg
    {71720,12120,57}, // machete.xbg
    {83720,12780,57}, // tanto_tp.xbg / tanto.xbg LOD1 (native tanto swipe draw, run 14616)
    {291640,49818,57}, // mk16.xbg
    {112928,22014,57}, // mk23.xbg
    {43360,11358,57}, // molotov.xbg
    {306360,55536,57}, // mp5.xbg
    {29632,8268,57}, // npc_knife.xbg
    {31872,9684,57}, // npc_knife_2.xbg
    {341536,65556,57}, // p416.xbg
    {636120,83850,57}, // pkm.xbg
    {119864,21396,57}, // proxmine.xbg
    {5920,1644,57}, // rock.xbg
    {201024,43572,57}, // rpg7.xbg
    {233152,49404,57}, // rpg7_rkt_menu.xbg
    {32128,5832,57}, // rpg7_rocket.xbg
    {304128,52560,57}, // sg551.xbg
    {59392,18300,57}, // shiv.xbg
    {127600,30018,57}, // spas12.xbg
    {186104,31002,57}, // stealthhunter.xbg
    {18032,2688,57}, // stealthhunter_barrel_long.xbg
    {14768,2172,57}, // stealthhunter_barrel_short.xbg
    {15360,2724,57}, // stealthhunter_fastloader.xbg
    {268464,59526,57}, // svd.xbg
    {8160,1476,57}, // svd_highpowered.xbg
    {23648,4098,57}, // svd_illuminated.xbg
    {71712,11892,57}, // blackhawk_sidemount_01.xbg
    {78080,13800,57}, // tripod_01.xbg
    {37536,7080,57}, // tripodmount_01.xbg
    {20128,3288,57}, // turret_armor_01.xbg
    {27456,5592,57}, // turret_gunmount.xbg
    {247392,45756,57}, // u100.xbg
    {41824,13176,57}, // ulc_tribalknife.xbg
    {119488,26364,57}, // ulc_type10flaregun.xbg
    {157984,35322,57}, // vz61.xbg
    {213320,34200,57}, // welding_torch.xbg
};
inline bool Model(const Geometry& draw)
{
    for(const auto& p:kModels)if(draw.vertexBytes==p.vertexBytes&&draw.indexBytes==p.indexBytes&&draw.format==p.format)return true;
    return false;
}
inline bool Present(const Geometry& draw)
{
    for(const auto& p:kProfiles)if(draw.vertexBytes==p.vertexBytes&&draw.indexBytes==p.indexBytes&&
       draw.stride==p.stride&&draw.offset==p.offset&&draw.format==p.format&&draw.start==p.start&&draw.count==p.count)return true;
    return false;
}
}
