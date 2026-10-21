#pragma once

// Saveable one-way refusal identifiers; runtime contact pointers belong to the current physics world.
// Portable supplement to the unchanged Snapshot28 wire layout. Only refused
// platform/body memberships are needed; newly born contacts default to allowed.
struct OneWaySavedPairNative {
    int platform, fixture, other;
};

struct OneWayCheckpointNative {
    u32 magic, version, snapshot;
    int count;
    OneWaySavedPairNative pairs[4096];
    u32 checksum;
};

OneWayCheckpointNative oneWayCheckpointNative;
void captureOneWayCheckpointNative(u32 snapshot);
void restoreOneWayCheckpointNative(u32 snapshot);
bool readOneWayCheckpointNative(const char *path, u32 snapshot);
bool writeOneWayCheckpointNative(const char *path);
