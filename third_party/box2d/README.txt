Box2D 2.3.1 by Erin Catto and contributors.
Historical source only. Active builds use ../box2d220 (Box2D 2.2.0), matching
the b2_version exported by the supplied libBeansQuest.so.
Source: https://github.com/erincatto/box2d/tree/v2.3.1
Original zlib license: LICENSE.txt and each source header.
Local modifications: b2Math.h uses x87 helpers; b2Settings.cpp uses Win32 heap/logging; b2BroadPhase.h uses a local in-place pair sort; b2Rope.cpp uses the local exponential helper. These adaptations keep the game independent of modern CRT DLLs on Windows XP. Global C++ constructors are called by the game's explicit startup routine.
