// Inspiration: https://www.codeproject.com/Articles/856020/Draw-Behind-Desktop-Icons-in-Windows-plus
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Threading;
using static PInvoke.User32;

WindowMessage DoMagicwindowStuff = (WindowMessage)0x052C;
IntPtr progman = CallWithErrorThrowing(() => FindWindow("Progman", null));

// Send magic message to create a window at the Z-level we want (between icons and wallpaper)
CallWithErrorThrowing(() => SendMessageTimeout(
    progman,
    DoMagicwindowStuff,
    IntPtr.Zero,
    IntPtr.Zero,
    SendMessageTimeoutFlags.SMTO_NORMAL,
    1000,
    out IntPtr _));

IntPtr theMagicWindow = IntPtr.Zero;

CallWithErrorThrowing(() => EnumWindows(
    FindCorrectWorkerW,
    IntPtr.Zero) ? new IntPtr(1) : IntPtr.Zero);

if (theMagicWindow == IntPtr.Zero)
{
    // Seems to happen over remote desktop; should abort from the start...
    throw new Exception("Failed to enum the magic window");
}

SafeDCHandle deviceContext = GetDCEx(theMagicWindow, IntPtr.Zero, DeviceContextValues.DCX_LOCKWINDOWUPDATE | DeviceContextValues.DCX_CACHE | DeviceContextValues.DCX_WINDOW);

if (deviceContext == SafeDCHandle.Null)
{
    throw new Exception("Failed to get a device context");
}

RotateColors(deviceContext);

_ = ReleaseDC(theMagicWindow, deviceContext.HWnd);

// Spy shows the follow magic messages (wparam/lparam) as part of backgroun cycle
// one of them kills the window
// 0/0          seems to start it; used above
// 5/big numer  ???
// 4/0          doest seem to kill it
// 8/big number ???
// 3/0          caused explorer to crash
CallWithErrorThrowing(() => SendMessageTimeout(
    progman,
    DoMagicwindowStuff,
    new IntPtr(3),
    IntPtr.Zero,
    SendMessageTimeoutFlags.SMTO_NORMAL,
    1000,
    out IntPtr _));

bool FindCorrectWorkerW(IntPtr hWnd, IntPtr _)
{
    // We enumerate all Windows until we find one that has the SHELLDLL_DefView 
    // as a child. If we found that window, we take its next sibling and assign it to workerw.
    IntPtr window = FindWindowEx(hWnd, IntPtr.Zero, "SHELLDLL_DefView", string.Empty);

    if (window != IntPtr.Zero)
    {
        // Gets the WorkerW Window after the current one.
        theMagicWindow = FindWindowEx(IntPtr.Zero, hWnd, "WorkerW", string.Empty);
    }

    // Always return true to indicate we didn't screw up processing; don't need to special case a fast 'already found it' result
    return true;
}

IntPtr CallWithErrorThrowing(Func<IntPtr> function)
{
    IntPtr result = function();
    if (result == IntPtr.Zero)
    {
        int error = Marshal.GetLastWin32Error();
        throw new Exception($"Failed with Win32 error {error}");
    }

    return result;
}

void RotateColors(SafeDCHandle deviceContext)
{
    using (Graphics gr = Graphics.FromHdc(deviceContext.DangerousGetHandle()))
    {

        // Use the Graphics instance to draw a white rectangle in the upper 
        // left corner. In case you have more than one monitor think of the 
        // drawing area as a rectangle that spans across all monitors, and 
        // the 0,0 coordinate being in the upper left corner.
        //g.FillRectangle(new SolidBrush(Color.BlueViolet), 0, 0, 500, 500);

        const int numSteps = 512;
        const double max = 2 * Math.PI;
        const double stepSize = max / numSteps;
        const double center = 255f / 2;
        const double rPhase = 0;
        const double gPhase = 2 * Math.PI / 3;
        const double bPhase = 2 * Math.PI / 3 * 2;
        bool keepLooping = true;
        while (keepLooping)
        {
            for (double theta = 0; theta <= max && keepLooping; theta += stepSize)
            {
                byte r = (byte)(Math.Sin(theta + rPhase) * center + center);
                byte g = (byte)(Math.Sin(theta + gPhase) * center + center);
                byte b = (byte)(Math.Sin(theta + bPhase) * center + center);


                gr.FillRectangle(new SolidBrush(Color.FromArgb(r, g, b)), 0, 0, 500, 500);
                Thread.Sleep(10);
                while (Console.KeyAvailable)
                {
                    var key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.Q)
                    {
                        Console.WriteLine("Exiting...");
                        keepLooping = false;
                        break;
                    }
                }
            }
        }
    }
}
