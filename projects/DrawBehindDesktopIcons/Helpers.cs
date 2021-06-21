namespace DrawBehindDesktopIcons
{
    using System.Runtime.InteropServices;
    using System;
    using static PInvoke.User32;
    using System.Drawing;
    using System.Threading;
    using PInvoke;

    internal static class Helpers
    {
        public static IntPtr CallWin32(Func<IntPtr> function)
        {
            IntPtr result = function();
            if (result == IntPtr.Zero)
            {
                ThrowLoastWin32();
            }

            return result;
        }

        public static void ThrowLoastWin32()
        {
            int error = Marshal.GetLastWin32Error();
            throw new Exception($"Failed with Win32 error {error}");
        }

        public static IDesktopWallpaper CreateIDesktopWallpaper()
        {
            var d = new DesktopWallpaper();
            var i = (IDesktopWallpaper)d;

            return i;
        }

        public static IntPtr FindMagicWindow()
        {
            IntPtr theMagicWindow = IntPtr.Zero;

            Console.Write("Trying to find that window now...");
            bool enumResult = EnumWindows((IntPtr hWnd, IntPtr _) =>
                {
                    // We enumerate all Windows until we find one that has the SHELLDLL_DefView 
                    // as a child. If we found that window, we take its next sibling.
                    IntPtr window = FindWindowEx(hWnd, IntPtr.Zero, "SHELLDLL_DefView", string.Empty);

                    if (window != IntPtr.Zero)
                    {
                        // Gets the WorkerW Window after the current one.
                        theMagicWindow = FindWindowEx(IntPtr.Zero, hWnd, "WorkerW", string.Empty);
                    }

                    // Always return true to indicate we didn't screw up processing; don't need to special case a fast 'already found it' result
                    return true;
                },
                IntPtr.Zero);

            if (!enumResult)
            {
                ThrowLoastWin32();
            }

            if (theMagicWindow == IntPtr.Zero)
            {
                // Seems to happen over remote desktop; should abort from the start...
                throw new Exception("Failed to enum the magic window");
            }

            Console.WriteLine($"0x{theMagicWindow:X}");
            return theMagicWindow;
        }

        public static void RotateColors(IntPtr hwnd)
        {
            SafeDCHandle deviceContext = GetDCEx(
                hwnd,
                IntPtr.Zero,
                DeviceContextValues.DCX_LOCKWINDOWUPDATE | DeviceContextValues.DCX_CACHE | DeviceContextValues.DCX_WINDOW);

            if (deviceContext == SafeDCHandle.Null)
            {
                throw new Exception("Failed to get a device context");
            }

            bool rectResult = GetWindowRect(hwnd, out RECT rect);

            if (!rectResult)
            {
                ThrowLoastWin32();
            }

            Console.WriteLine("Press Q to quit");
            using (Graphics gr = Graphics.FromHdc(deviceContext.DangerousGetHandle()))
            {
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

                        gr.FillRectangle(new SolidBrush(Color.FromArgb(r, g, b)), rect.top, rect.left, rect.right, rect.bottom);
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

            _ = ReleaseDC(hwnd, deviceContext.HWnd);
        }
    }
}
