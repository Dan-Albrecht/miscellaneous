namespace DrawBehindDesktopIcons
{
    using PInvoke;
    using System;
    using System.Drawing;
    using System.Numerics;
    using System.Runtime.InteropServices;
    using System.Threading;
    using Windows.System;
    using Windows.UI.Composition;
    using Windows.UI.Composition.Desktop;
    using WinRT;
    using static PInvoke.User32;
    using static DrawBehindDesktopIcons.VisualLayer;
    using System.Threading.Tasks;

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

                        gr.FillRectangle(new SolidBrush(Color.FromArgb(r, g, b)), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

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

        // https://docs.microsoft.com/en-us/windows/uwp/composition/visual-layer
        // https://github.com/microsoft/CsWinRT/blob/master/docs/interop.md
        public static void DoVisualLayerStuff(IntPtr hwnd)
        {
            _ = CreateDispatcherQueueControllerForCurrentThread();

            bool rectResult = GetWindowRect(hwnd, out RECT rect);

            if (!rectResult)
            {
                ThrowLoastWin32();
            }

            hwnd = CreateWindow(
                "STATIC",
                "Animation Window",
                WindowStyles.WS_VISIBLE | WindowStyles.WS_CHILD,
                rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                hwnd, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);

            Console.WriteLine($"Created new child: 0x{hwnd:X}");

            Compositor compositor = new();
            var ptr = compositor.As<ICompositorDesktopInterop>();
            ptr.CreateDesktopWindowTarget(hwnd, false, out IntPtr desktopWindowTargetAbi);

            //DesktopWindowTarget dwt = ABI.Windows.UI.Composition.Desktop.DesktopWindowTarget.FromAbi(desktopWindowTargetAbi);
            CompositionTarget cwt = ABI.Windows.UI.Composition.CompositionTarget.FromAbi(desktopWindowTargetAbi);
            ContainerVisual containerVisual = compositor.CreateContainerVisual();
            cwt.Root = containerVisual;

            Console.WriteLine("Press Q to quit");
            PumpMessageUntilQKey(hwnd, (nextValue) => XXX(nextValue, compositor, containerVisual));
        }

        private static double XXX(double theta, Compositor compositor, ContainerVisual containerVisual)
        {
            const int numSteps = 512;
            const double max = 2 * Math.PI;
            const double stepSize = max / numSteps;
            const double center = 255f / 2;
            const double rPhase = 0;
            const double gPhase = 2 * Math.PI / 3;
            const double bPhase = 2 * Math.PI / 3 * 2;

            byte r = (byte)(Math.Sin(theta + rPhase) * center + center);
            byte g = (byte)(Math.Sin(theta + gPhase) * center + center);
            byte b = (byte)(Math.Sin(theta + bPhase) * center + center);

            var last = compositor.CreateSpriteVisual();
            last.Size = new Vector2(400, 400);
            last.Brush = compositor.CreateColorBrush(Windows.UI.Color.FromArgb(255, r, g, b));
            last.Offset = new Vector3(0, 800, 0);
            containerVisual.Children.RemoveAll();
            containerVisual.Children.InsertAtTop(last);

            Task.Delay(0).ConfigureAwait(false).GetAwaiter().GetResult();

            return theta += stepSize;
        }

        private static void PumpMessageUntilQKey(IntPtr hwnd, Func<double, double> theThing)
        {
            unsafe
            {
                var msg = new MSG();
                bool keepLooping = true;
                double nextValue = 0;

                while (keepLooping)
                {
                    while (PeekMessage(&msg, hwnd, WindowMessage.WM_NULL, WindowMessage.WM_NULL, PeekMessageRemoveFlags.PM_REMOVE))
                    {
                        if (msg.message == WindowMessage.WM_QUIT)
                        {
                            Console.WriteLine("Got a WM_QUIT somehow. Going with it...");
                            keepLooping = false;
                            break;
                        }

                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

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

                    nextValue = theThing(nextValue);
                }
            }
        }
    }
}
