#pragma once

// Shared water contacts behavior used by the Windows reconstruction.

// WaterArea Begin/End0x32dc18/0x32dab8 stores a unique PhysicsObject set.
// Every End removes that object, even if another fixture remains touching.
// Body pointers distinguish detector proxies and independent LevelTask worlds.
// Clear the transient associations before destroying a world; Load reuses them
// when the same body/contact lifetime continues, as the original does.
struct NativeWaterMember {
    b2World *world;
    b2Body *water;
    b2Body *other;
    NativeWaterMember *next;
};

NativeWaterMember *nativeWaterMembers;

void nativeWaterContact(b2Contact *contact, bool begin) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++)
        if (fixtures[side]->GetUserData() == (void *)7) {
            b2Body *water = fixtures[side]->GetBody();
            b2Body *other = fixtures[1 - side]->GetBody();
            if (!other->GetUserData())
                continue;
            NativeWaterMember **link = &nativeWaterMembers;
            while (*link && ((*link)->water != water || (*link)->other != other))
                link = &(*link)->next;
            if (begin) {
                if (*link)
                    continue;
                NativeWaterMember *node =
                    (NativeWaterMember *)HeapAlloc(GetProcessHeap(), 0, sizeof(NativeWaterMember));
                if (!node)
                    fatal("Water contact allocation failed.");
                node->world = water->GetWorld();
                node->water = water;
                node->other = other;
                node->next = 0;
                *link = node;
            } else if (*link) {
                NativeWaterMember *node = *link;
                *link = node->next;
                HeapFree(GetProcessHeap(), 0, node);
            }
        }
}

void clearNativeWaterWorld(b2World *world) {
    NativeWaterMember **link = &nativeWaterMembers;
    while (*link) {
        NativeWaterMember *node = *link;
        if (node->world == world) {
            *link = node->next;
            HeapFree(GetProcessHeap(), 0, node);
        } else
            link = &node->next;
    }
}
