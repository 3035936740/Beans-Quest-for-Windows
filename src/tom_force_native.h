#pragma once

// Shared tom force behavior used by the Windows reconstruction.

// Tom PrePhysics0x311ad4..0x311c64 stores the selected velocity first,
// then applies mass * controller gravity at the body origin. Box2D integrates
// it after PreSolve, together with previously accumulated fluid forces.
// Flying suppresses this force only BELOW the cap, not at equality.
bool nativeTomGravityEnabled(bool flying, float selectedVy, float flyCap) {
    return !flying || selectedVy >= flyCap;
}

void nativeTomGravityForce(b2Body &body, float gravity, bool flying, float selectedVy,
                           float flyCap) {
    if (nativeTomGravityEnabled(flying, selectedVy, flyCap))
        body.ApplyForce(b2Vec2(0, body.GetMass() * gravity * metres), body.GetPosition());
}
