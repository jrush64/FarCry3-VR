// Personal body/hand calibration, separate from existing weapon/reload settings.
struct CalibrationProfile136 {
 char name[48]="Player";bool valid=false;float shoulder[3]={.269f,-.24f,-.304f};float reach=.60f,upperLimit=2.f;bool forwardFit=false;float yaw=0,reachDelta[2]={};

};
CalibrationProfile136 g_calibProfiles136[3],g_calibDraft136;int g_calibSlot136=0;bool g_calibEnabled136=false,g_calibPreview136=false;
void LoadCalibration136(const wchar_t*);
void CalibrationMenu136();void TickCalibration136();void RenderCalibration136(ID3D11Texture2D*,ID3D11Texture2D*);
void CancelCalibration136();

void CalibrationFonts139();
