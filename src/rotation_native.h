#pragma once

// Updates rotating mechanisms and their original gravity-dependent motion.

// Rotating::OnActivate0x2e3cc8..cd4 initializes(actor,pivot,anchor), and
//0x2e3d34 writes rotation-speed*DegToRad unchanged. The port joint order
// is(pivot,actor), so its motor target must be negated: motor speed is wB-wA.
float nativeRotatingMotorSpeed(const Props &p) {
    return -p.num("rotation-speed", 90) * 0.017453293f;
}

// Native0x2e3d84 supplies1, although every authored placed rotor overrides150.
float nativeRotatingMotorPower(const Props &p) {
    return p.num("rotation-power", 1);
}
