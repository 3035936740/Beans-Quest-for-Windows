#pragma once

// Shared third-party attribution shown by settings and ending credits in the Windows port.
// Text-only replacements for the two authored third-party credit sections.
// Original authors, scene geometry, typography and ending timing stay intact.
const char *windowsPortCreditCaption(u64 scene, u64 node, const char *original) {
    if (scene == 8466014814437108852ULL && node == 7424469102921688ULL)
        return "|FFFFFF|Art\n|CCCCCC|James Greenaway\n\n|FFFFFF|Code\n|CCCCCC|Ivan Neeson\n|CCCCCC|Tom Greenaway\n\n\n\n"
               "|FFFFFF|3rd Party - Windows Port\n"
               "|CCCCCC|Box2D 2.2.0 - Erin Catto\n"
               "|CCCCCC|Bitstream Vera - Bitstream Inc.\n"
               "|CCCCCC|Win32 / OpenGL 1.1 / WinMM\n"
               "|CCCCCC|Media Foundation / MCI\n"
               "|CCCCCC|Licenses: THIRD_PARTY_NOTICES.txt\n\n"
               "Bean's Quest is Copyright Kumobius Pty Ltd";
    if (scene != 137502285691ULL)
        return original;
    switch (node) {
    case 3338695805181101ULL:
        return "3rd Party - Windows Port";
    case 6903175912663783ULL:
        return "Box2D 2.2.0";
    case 9866763518596201ULL:
        return "Erin Catto and contributors";
    case 4967306077651177ULL:
        return "Bitstream Vera Fonts";
    case 9008408490901651ULL:
        return "Bitstream Inc.";
    case 8988470611758986ULL:
        return "Windows APIs";
    case 1143186199308468ULL:
        return "Microsoft Windows";
    case 1306399108642027ULL:
        return "Graphics / Audio";
    case 9108887204814361ULL:
        return "OpenGL 1.1 / WinMM";
    case 5274822065804619ULL:
        return "Video Playback";
    case 1267943937636961ULL:
        return "Media Foundation / MCI";
    case 9260610197510234ULL:
        return "Licenses";
    case 8500185548000906ULL:
        return "THIRD_PARTY_NOTICES.txt";
    }
    return original;
}
