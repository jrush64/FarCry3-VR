fc3haptic302::Queue g_feedback302;
fc3gunfire339::Queue g_gunfire339;
volatile LONG g_gunfireStrength339=65;
volatile LONG g_feedbackStrength302=100;
bool g_feedbackLoaded302=false,g_feedbackWasLive302=false,g_feedbackSpecific302=false;
DWORD g_feedbackSpecificAt302=0;

void LoadFeedback302(const wchar_t* explicitPath=nullptr) {
    if (g_feedbackLoaded302 && !explicitPath) return;
    wchar_t path[MAX_PATH];
    if (explicitPath) wcsncpy_s(path,explicitPath,_TRUNCATE);
    else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
    wchar_t text[32]; GetPrivateProfileStringW(L"Haptics",L"Strength",L"100",text,32,path);
    wchar_t* end=nullptr; const long value=wcstol(text,&end,10);
    const int percent=end!=text && !*end && value>=0 && value<=100 ? int(value) : 100;
    InterlockedExchange(&g_feedbackStrength302,percent);g_feedbackLoaded302=true;
    GetPrivateProfileStringW(L"Haptics",L"GunfireStrength",L"65",text,32,path);
    end=nullptr;const long gunfire=wcstol(text,&end,10);
    InterlockedExchange(&g_gunfireStrength339,end!=text&&!*end&&gunfire>=0&&gunfire<=100?int(gunfire):65);
}
void ClearFeedback302() {
    g_feedback302.SetLive(GetTickCount(),false);
    g_gunfire339.SetLive(GetTickCount64(),false);
    InterlockedExchange(&g_interactionPulse285,0);
    InterlockedExchange(&g_macheteHit121.stamp,0);
}
XrResult XRAPI_CALL ApplyFeedback302(XrSession session,const XrHapticActionInfo* info,const XrHapticBaseHeader* feedback) {
    if (!p_xrApplyHapticFeedback || !feedback || feedback->type!=XR_TYPE_HAPTIC_VIBRATION)
        return XR_ERROR_VALIDATION_FAILURE;
    XrHapticVibration vibration=*reinterpret_cast<const XrHapticVibration*>(feedback);
    vibration.amplitude=fc3haptic302::Amplitude(vibration.amplitude,int(InterlockedCompareExchange(&g_feedbackStrength302,0,0)));
    if (vibration.amplitude<=0) return XR_SUCCESS;
    return p_xrApplyHapticFeedback(session,info,reinterpret_cast<const XrHapticBaseHeader*>(&vibration));
}
// Only called on the XR thread AFTER action synchronization. No producer calls XR.
void TickFeedback302(XrResult synced) {
    LoadFeedback302();
    const DWORD now=GetTickCount();
    const bool live=g_actionsReady&&g_sessionRunning&&g_session!=XR_NULL_HANDLE&&synced==XR_SUCCESS&&
        p_xrApplyHapticFeedback&&InterlockedCompareExchange(&g_feedbackStrength302,0,0)>0;
    const bool continuous=g_feedback302.SetLive(now,live);
    g_gunfire339.SetLive(GetTickCount64(),live&&InterlockedCompareExchange(&g_gunfireStrength339,0,0)>0);
    if (!continuous) {
        g_feedbackSpecific302=false;
        InterlockedExchange(&g_interactionPulse285,0);
        InterlockedExchange(&g_macheteHit121.stamp,0);
    }
    if (!live) {
        // Each request is finite even if a runtime refuses stop while unfocused.
        if (g_feedbackWasLive302 && p_xrStopHapticFeedback && g_sessionRunning && g_session) {
            for (int hand=0;hand<2;++hand) {
                XrHapticActionInfo info={XR_TYPE_HAPTIC_ACTION_INFO};
                info.action=g_actions[hand==0?kHapticRight:kHapticLeft];
                if (info.action) p_xrStopHapticFeedback(g_session,&info);
            }
        }
        g_feedbackWasLive302=false;return;
    }
    g_feedbackWasLive302=true;
    // Specific hand feedback wins over a generic two-hand notification this tick.
    const LONG body=InterlockedExchange(&g_interactionPulse285,0);
    const bool hit=InterlockedCompareExchange(&g_macheteHit121.stamp,0,0)!=0;
    const auto shot=g_gunfire339.Take(fc3hands303::Read(),GetTickCount64());
    if(body||hit||shot.hand>=0){g_feedbackSpecific302=true;g_feedbackSpecificAt302=now;}
    if (!body && !hit && shot.hand<0 && (!g_feedbackSpecific302 || now-g_feedbackSpecificAt302>=45) && g_feedback302.Take(now)) Pulse();
    // Reload/holster and melee retain priority. Conflicting recoil is consumed,
    // not delayed until a later hand or weapon owns the controller.
    if(shot.hand>=0&&!(body&(1<<shot.hand))&&!(shot.hand==0&&hit)){
        XrHapticActionInfo info={XR_TYPE_HAPTIC_ACTION_INFO};info.action=g_actions[shot.hand==0?kHapticRight:kHapticLeft];
        XrHapticVibration pulse={XR_TYPE_HAPTIC_VIBRATION};pulse.duration=shot.duration;pulse.frequency=XR_FREQUENCY_UNSPECIFIED;
        pulse.amplitude=fc3haptic302::Amplitude(shot.amplitude,int(InterlockedCompareExchange(&g_gunfireStrength339,0,0)));
        if(info.action)ApplyFeedback302(g_session,&info,reinterpret_cast<const XrHapticBaseHeader*>(&pulse));
    }
    for (int hand=0;hand<2;++hand) if (body&(1<<hand)) {
        XrHapticActionInfo info={XR_TYPE_HAPTIC_ACTION_INFO};info.action=g_actions[hand==0?kHapticRight:kHapticLeft];
        if (!info.action || (hand==0 && hit)) continue;
        XrHapticVibration pulse={XR_TYPE_HAPTIC_VIBRATION};pulse.duration=25000000;pulse.amplitude=.22f;pulse.frequency=XR_FREQUENCY_UNSPECIFIED;
        ApplyFeedback302(g_session,&info,reinterpret_cast<const XrHapticBaseHeader*>(&pulse));
    }
    fc3hit121::Consume(g_macheteHit121,now,true,true,g_session,g_actions[kHapticRight],ApplyFeedback302);
}
