// From work/z93_bolt_path.py (captures/weapons6-z93-96724, kZ93ClipBolt), see work/z93-report.md.
// The physical stroke = the OPENING half of the captured cycle, keys 4 (rest) .. 10 (handle up 94 deg) .. 16 (bolt 150 mm back),
// parametrised by s_open = arc length of the right-hand grasp point along that path (0.265 m total; handle-up corner at 0.4270).
// DIFFERENCE TO THE R700: the handle goes up first (rotation ~94 deg, no slide), THEN the bolt slides 149.9 mm with the handle held up; the closing half does NOT retrace the opening path exactly (max 33 mm off).
// The grasp point kZ93BoltTipLocal is the REAL PALM of Jason (bind-space wrist point (.591,.011,1.149) + (.0658,.0481,-.1174)) in bolt-local coordinates: the animation hand is NOT rigid on the handle, the palm is its best pivot.
const float kZ93BoltTipLocal[3]={0.0399103f,0.0629949f,-0.0565905f};          // grasp point = real palm, bolt-local
const float kZ93BoltTipRestFromFrame[3]={0.0399129f,0.108889f,0.0474764f};  // rel FRAME, bolt at rest
const float kZ93BoltTipLiftedFromFrame[3]={0.0534792f,0.108602f,0.148057f};
const float kZ93BoltTipBackFromFrame[3]={0.047793f,-0.0410104f,0.154176f};
const float kZ93GrabBoxCenter[3]={0.0543677f,0.108745f,0.0977665f};            // BOLT box default: centred where the PALM is on the handle (centre of the real-palm arc rest..lifted)
const float kZ93GrabBoxSize[3]={0.0889096f,0.0602872f,0.16058f};              // full extents: the rest..lifted real-palm arc + 3 cm
const float kZ93GrabBoxCenterR700Recipe[3]={-0.0366837f,0.103044f,0.0921297f};  // reference only: literal R700 recipe (wrist-point arc centre + wrist->palm vector); too wide here
const float kZ93BoltWristTipLocal[3]={-0.0396831f,-0.0414276f,-0.121186f};        // reference: the R700-style WRIST point in bolt-local (NOT on the Z93 handle)
const float kZ93LeftPalmFromFrame[3]={-0.0392041f,0.294135f,0.0359932f};         // left palm on the forend (idle), rel FRAME
const int kZ93StrokeFirstKey=4,kZ93StrokeBackKey=16,kZ93StrokeCount=13;       // kZ93ClipBolt keys 4..16
const int kZ93StrokeHandKey=10;                                                  // first key with the handle up and the hand on it (fingers from here on)
const float kZ93StrokeS[13]={0.f,0.00450121f,0.037095f,0.102473f,0.219559f,0.307224f,0.426956f,0.448901f,0.583476f,0.717648f,0.947964f,0.990789f,1.f};
const float kZ93StrokeLiftedS=0.426956f;                                           // the corner: lift done, slide begins
// Reload note: the game's EMPTY reload slides the bolt 149.9 mm straight (rotation 0, no hand) after the magazine is seated; the PARTIAL reload does not move the bolt.
