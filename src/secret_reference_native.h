#pragma once

// Resolves authored secret-reveal references and their shared layer effects.

// A SecretRevealer is an App::Box, whose Activate0x130e5c creates a
// static, zero-density sensor at the original box bounds. This is separate
// from BoxPhysicsBehaviour and must not inherit adjusted actor hit bounds.
b2Body *createNativeSecretBody(b2World &world, Entity &e, int index) {
    Rect r = e.local;
    if (r.r <= r.l || r.t <= r.b)
        return 0;
    b2BodyDef definition;
    definition.type = b2_staticBody;
    definition.position.Set(e.state.x * metres, e.state.y * metres);
    definition.angle = e.state.angle * 0.017453293f;
    definition.userData = (void *)(index + 1);
    b2Body *body = world.CreateBody(&definition);
    b2PolygonShape shape;
    shape.SetAsBox((r.r - r.l) * .5f * metres, (r.t - r.b) * .5f * metres,
                   b2Vec2((r.l + r.r) * .5f * metres, (r.b + r.t) * .5f * metres), 0);
    b2FixtureDef fixture;
    fixture.shape = &shape;
    fixture.isSensor = true;
    fixture.density = 0;
    fixture.friction = .2f;
    fixture.restitution = 0;
    // Native Box category1 is the loose-object category, mapped to4 by
    // this port; mask0xffff/group0 are unmodified native Box defaults.
    fixture.filter.categoryBits = 4;
    fixture.filter.maskBits = 0xffff;
    fixture.filter.groupIndex = 0;
    body->CreateFixture(&fixture);
    body->SetActive(!e.state.gone);
    return body;
}

int nativeSecretContacts(b2Body *marker, b2Body *tom) {
    if (!marker || !tom || !marker->IsActive() || !tom->IsActive())
        return 0;
    int count = 0;
    for (b2ContactEdge *edge = marker->GetContactList(); edge; edge = edge->next)
        if (edge->other == tom && edge->contact->IsTouching())
            count++;
    return count;
}

// Hide0x2f2dbc/Show0x2f3244 change one layer reference count, not an
// opacity per marker. Recompute that count from the live solver contacts
// instead of retaining fixture pointers or serializing callback counts.
// OnUpdate0x2f160c advances that layer once at4/s. Existing checkpointed
// aux stores the shared fade on every member, requiring no save extension.
void advanceNativeSecretLayers(Entity *things, int count, b2Body **physics, int tomIndex,
                               float dt) {
    b2Body *tom = tomIndex >= 0 && tomIndex < count ? physics[tomIndex] : 0;
    for (int i = 0; i < count; i++)
        if (things[i].kind == SECRET) {
            bool first = true;
            for (int j = 0; j < i; j++)
                if (things[j].kind == SECRET && things[j].layer == things[i].layer) {
                    first = false;
                    break;
                }
            if (!first)
                continue;
            int references = 0;
            float fade = 0;
            for (int j = i; j < count; j++)
                if (things[j].kind == SECRET && things[j].layer == things[i].layer) {
                    fade = maxf(fade, clamp(things[j].state.aux, 0, 1));
                    if (!things[j].state.gone)
                        references += nativeSecretContacts(physics[j], tom);
                }
            fade = approach(fade, references > 0 ? 1.0f : 0.0f, maxf(0, dt) * 4);
            for (int j = i; j < count; j++)
                if (things[j].kind == SECRET && things[j].layer == things[i].layer)
                    things[j].state.aux = fade;
        }
}
