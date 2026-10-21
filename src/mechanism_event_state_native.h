#pragma once

// Saveable mechanism event phase without live contact pointers.
// Both authored mechanism kinds leave aux unused. Reserve a finite tagged
// range for saved events; ordinary/legacy0,+1,-1 are never event payloads.
float encodeMechanismEventAuxNative(int value) {
    return -65536.0f - (float)value;
}

int decodeMechanismEventAuxNative(float aux) {
    float value = -aux - 65536.0f;
    if (value >= 1 && value <= 34464) {
        int event = (int)value;
        if ((float)event == value)
            return event;
    }
    return 0;
}
