// From work/r700_bolt_path.py (captures/weapons5-95632, kR700ClipBolt), see work/r700-bolt-physical.md.
// The physical stroke = the OPENING half of the captured cycle, keys 8 (rest) .. 15 (handle lifted 65 deg) .. 22 (bolt 70 mm back),
// parametrised by s_open = arc length of the right-hand grasp point along that path (0.243 m total; lifted at 0.5985).
// The closing half retraces the same path (closing keys project onto it within 1.3 mm), so one position-driven fraction serves both ways.
// The grasp point is the right palm in bolt-local coordinates (rigid on the handle to <1.5 mm for keys 14..28; 128 mm lever arm).
// Right palm = right wrist x (+.591,.011,1.149) (the right arm skin is mirrored; -.591 is the LEFT hand's palm offset).
const float kR700BoltTipLocal[3]={-0.00234185f,-0.0612734f,-0.128204f};          // grasp point, bolt-local
const float kR700BoltTipRestFromFrame[3]={-0.00146374f,0.143944f,-0.0550927f};  // rel FRAME, bolt at rest
const float kR700BoltTipLiftedFromFrame[3]={0.116178f,0.143944f,0.0170061f};
const float kR700BoltTipBackFromFrame[3]={0.0720126f,0.0739379f,-0.0335789f};
const float kR700GrabBoxCenter[3]={0.0573569f,0.143945f,-0.0190433f};            // BOLT box default (INSERT "charge" box of family 14)
const float kR700GrabBoxSize[3]={0.1776414f,0.0600008f,0.1320988f};              // full extents: the rest..lifted arc + 3 cm
const float kR700LeftPalmFromFrame[3]={-0.044789f,0.360949f,0.0141573f};         // left palm on the forend (idle), rel FRAME
const int kR700StrokeFirstKey=8,kR700StrokeBackKey=22,kR700StrokeCount=15;       // kR700ClipBolt keys 8..22
const int kR700StrokeHandKey=15;                                                  // first key with the hand rigid on the handle (fingers from here on)
const float kR700StrokeS[15]={0.f,0.0627723f,0.142714f,0.254314f,0.400234f,0.512216f,0.57181f,0.598499f,0.5985f,0.620826f,0.693333f,0.828488f,0.93698f,0.994552f,1.f};
const float kR700StrokeLiftedS=0.5985f;                                           // the corner: lift done, slide begins
// Reload note: the game's own reload opens the bolt further (75.4 deg, same 70 mm) with the hand elsewhere; the shot stroke does not
// cover it, which is why the EMPTY reload stays the game's own animation.
