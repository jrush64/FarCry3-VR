//367: use the actual curled finger skeleton, not the fixed debug palm point.
// Index and middle MCP/DIP pairs bound the channel through the closed fist.
// Coordinates are anatomical bind points; each is skinned by its own bone.
V3 CarFistCenter367(const float* bones,int side){
    V3 sum={};const double mirror=side? -1.:1.;
    for(int f=1;f<=2;++f)for(int k:{0,2}){
        const auto& chain=kFingers[f];const auto* j=chain.joints[k];
        sum=Add(sum,Apply(BoneMatrix(bones,chain.bones[k]),{j[0]*mirror,j[1],j[2]}));
    }
    return Scale(sum,.25);
}
// Same swing/twist distribution as the accepted ordinary arm renderer.
// Vehicle-only helper: no weapon/reload path is modified.
void CarForearmTwist367(const ArmJoints& joints,const Mat4& handMove,const Mat4& forearmMove,int side,Mat4* twistMoves){
    twistMoves[0]=twistMoves[1]=forearmMove;
    {
        Mat4 forearmInverse = {};
        const V3 e = Apply(forearmMove, { joints.e[0], joints.e[1], joints.e[2] }), w = Apply(forearmMove, { joints.w[0], joints.w[1], joints.w[2] });
        V3 axis = Sub(w, e); const double axisLength = Length(axis);
        if (axisLength > 1e-6 && Invert(forearmMove, forearmInverse))
        {
            axis = Scale(axis, 1 / axisLength);
            Mat4 twistInverse=forearmInverse,rigidInverse={};
            if(true&&Invert(g_armForearmRigid77[side],rigidInverse))twistInverse=rigidInverse;
            const Mat4 rel = Multiply(handMove, twistInverse); // longitudinal extension is not wrist twist
            const double* m = rel.m; double qw = 0, qx = 0, qy = 0, qz = 0; const double tr = m[0] + m[5] + m[10];
            if (tr > 0) { const double s = std::sqrt(tr + 1) * 2; qw = .25 * s; qx = (m[9] - m[6]) / s; qy = (m[2] - m[8]) / s; qz = (m[4] - m[1]) / s; }
            else if (m[0] > m[5] && m[0] > m[10]) { const double s = std::sqrt(1 + m[0] - m[5] - m[10]) * 2; qw = (m[9] - m[6]) / s; qx = .25 * s; qy = (m[1] + m[4]) / s; qz = (m[2] + m[8]) / s; }
            else if (m[5] > m[10]) { const double s = std::sqrt(1 + m[5] - m[0] - m[10]) * 2; qw = (m[2] - m[8]) / s; qx = (m[1] + m[4]) / s; qy = .25 * s; qz = (m[6] + m[9]) / s; }
            else { const double s = std::sqrt(1 + m[10] - m[0] - m[5]) * 2; qw = (m[4] - m[1]) / s; qx = (m[2] + m[8]) / s; qy = (m[6] + m[9]) / s; qz = .25 * s; }
            double twist = 2 * std::atan2(qx * axis.x + qy * axis.y + qz * axis.z, qw);
            if (twist > 3.14159265358979) twist -= 6.28318530717959; else if (twist < -3.14159265358979) twist += 6.28318530717959;
            const double share[2] = { .70, .35 };
            for (int k = 0; k < 2; ++k)
            {
                const double a = twist * share[k], c = std::cos(a), s = std::sin(a), t = 1 - c; const double u[3] = { axis.x, axis.y, axis.z };
                Mat4 roll = {}; roll.m[15] = 1;
                for (int r = 0; r < 3; ++r) for (int col = 0; col < 3; ++col) roll.m[r * 4 + col] = (r == col ? c : 0) + t * u[r] * u[col];
                roll.m[1] -= s * u[2]; roll.m[2] += s * u[1]; roll.m[4] += s * u[2]; roll.m[6] -= s * u[0]; roll.m[8] -= s * u[1]; roll.m[9] += s * u[0];
                const V3 turned = Apply(roll, w); roll.m[3] = w.x - turned.x; roll.m[7] = w.y - turned.y; roll.m[11] = w.z - turned.z; // about the moved wrist
                twistMoves[k] = Multiply(roll, forearmMove);
            }
        }
    }
}
