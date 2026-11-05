#ifdef FC3_AMMO_TEST
ImVec2 g_macheteSlider118[11],g_macheteSave118[11];
#endif
void MacheteColliderMenu118(const wchar_t* supplied)
{
    wchar_t path[MAX_PATH];ManualSettingsPath(path);if(supplied)wcscpy_s(path,supplied);static int result=0;
    ImGui::TextWrapped("Right grip + R3 draws/stows. Damage comes from the blue blade capsule touching a target while moving. No automatic attack animation or RT swipe. Position and rotation follow the blade's local axes.");
    ImGui::TextWrapped("Red: physics contact. Yellow: damage event sent to the contacted target. A damage event alone does not confirm health loss.");
    if(ImGui::Checkbox("Right-hand hit vibration",&g_macheteCollider118.haptics))result=0;
    ImGui::TextWrapped("Enemies take one hit per contact. Pull the blade clear before cutting again; holding it inside cannot repeat damage.");
    if(ImGui::Checkbox("Show blade contact capsule",&g_macheteCollider118.show))result=0;
    if(ImGui::Button("Save All - Machete collider"))result=SaveMacheteCollider118(path)?1:2;
    ImGui::SameLine();if(ImGui::Button("Reset collider")){g_macheteCollider118=MacheteColliderSettings118{};result=0;}
    ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Live edits");
    const char* labels[]={"Contact X","Contact Y","Contact Z","Rotate X","Rotate Y","Rotate Z","Full capsule length","Capsule radius","Minimum movement speed","Damage multiplier","Scenery repeat cooldown"};
    for(int n=0;n<11;++n){ImGui::PushID(11800+n);float& value=MacheteColliderField118(g_macheteCollider118,n);const bool cm=n<3||n==6||n==7;float shown=value*(cm?100.f:1.f);float lo=n<3?-50.f:n<6?-180.f:n==6?5.f:n==7?.5f:n==8?0.f:n==9?.1f:50.f;float hi=n<3?50.f:n<6?180.f:n==6?120.f:n==7?20.f:n==8?5.f:n==9?5.f:2000.f;
        ImGui::SetNextItemWidth(220*g_menuScale);if(ImGui::SliderFloat(labels[n],&shown,lo,hi,cm?"%.2f cm":n<6?"%.1f deg":n==8?"%.2f m/s":n==9?"%.2f x":"%.0f ms")){value=shown*(cm?.01f:1.f);result=0;}
#ifdef FC3_AMMO_TEST
        {auto p=ImGui::GetItemRectMin();g_macheteSlider118[n]=ImVec2(p.x+165*g_menuScale,p.y+ImGui::GetItemRectSize().y*.5f);}
#endif
        ImGui::SameLine();if(ImGui::Button("Save"))result=SaveMacheteCollider118(path,n)?1:2;
#ifdef FC3_AMMO_TEST
        {auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();g_macheteSave118[n]=ImVec2((a.x+b.x)*.5f,(a.y+b.y)*.5f);}
#endif
        ImGui::PopID();}
    ImGui::TextWrapped("Draw the machete before opening Insert to position the capsule on the blade. Damage is paused while Insert is open. Capsule position, rotation and size control the actual collision queries.");
}
