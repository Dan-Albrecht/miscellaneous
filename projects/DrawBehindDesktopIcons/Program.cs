// Inspiration: https://www.codeproject.com/Articles/856020/Draw-Behind-Desktop-Icons-in-Windows-plus
using DrawBehindDesktopIcons;
using System;
using static DrawBehindDesktopIcons.Helpers;
using static PInvoke.User32;

const WindowMessage DoMagicwindowStuff = (WindowMessage)0x052C;

bool dpiSet = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
if (!dpiSet)
{
    ThrowLoastWin32();
}

Console.Write("Finding Shell window...");
IntPtr shellWindow = CallWin32(() => GetShellWindow());
Console.WriteLine($"0x{shellWindow:X}");

Console.Write("Sending create magic window message...");

// Send magic message to create a window at the Z-level we want (between icons and wallpaper)
CallWin32(() => SendMessageTimeout(
    shellWindow,
    DoMagicwindowStuff,
    IntPtr.Zero,
    IntPtr.Zero,
    SendMessageTimeoutFlags.SMTO_NORMAL,
    1000,
    out IntPtr _));

Console.WriteLine("sent");

IntPtr theMagicWindow = FindMagicWindow();

Console.WriteLine("We're good to go");
Console.WriteLine("Press 1 to rotate via GDI, anything else to rotate via Visual Layer");
var key = Console.ReadKey(true);

if (key.KeyChar == '1')
{
    RotateColors(theMagicWindow);
}
else
{
    DoVisualLayerStuff(theMagicWindow);
}

Console.WriteLine("Blipping wallper to clear, this annoyingly takes a few seconds...");

// Best way I've found to get rid of the window...
IDesktopWallpaper desktopWallpaper = CreateIDesktopWallpaper();
desktopWallpaper.Enable(false);
desktopWallpaper.Enable(true);
