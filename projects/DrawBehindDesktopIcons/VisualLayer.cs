namespace DrawBehindDesktopIcons
{
    using System;
    using System.Runtime.InteropServices;
    using Windows.System;
    using Windows.UI.Composition;

    // https://docs.microsoft.com/en-us/windows/apps/desktop/modernize/using-the-visual-layer-with-windows-forms

    [ComImport]
    [Guid("29E691FA-4567-4DCA-B319-D0F207EB6807")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICompositorDesktopInterop
    {
        void CreateDesktopWindowTarget(IntPtr hwndTarget, bool isTopmost, out IntPtr idesktopWindowTarget);
    }

    [ComImport]
    [Guid("A1BEA8BA-D726-4663-8129-6B5E7927FFA6")]
    [InterfaceType(ComInterfaceType.InterfaceIsIInspectable)]
    internal interface ICompositionTarget
    {
        Visual Root
        {
            get;
            set;
        }
    }

    internal enum DISPATCHERQUEUE_THREAD_APARTMENTTYPE
    {
        DQTAT_COM_NONE = 0,
        DQTAT_COM_ASTA = 1,
        DQTAT_COM_STA = 2
    }

    internal enum DISPATCHERQUEUE_THREAD_TYPE
    {
        DQTYPE_THREAD_DEDICATED = 1,
        DQTYPE_THREAD_CURRENT = 2
    }

    internal struct DispatcherQueueOptions
    {
        public int dwSize;
        public DISPATCHERQUEUE_THREAD_TYPE threadType;
        public DISPATCHERQUEUE_THREAD_APARTMENTTYPE apartmentType;
    }

    internal static class VisualLayer
    {

        [DllImport(
            "CoreMessaging.dll",
            EntryPoint = "CreateDispatcherQueueController",
            SetLastError = true,
            CharSet = CharSet.Unicode,
            ExactSpelling = true,
            CallingConvention = CallingConvention.StdCall
            )]
        static extern uint CreateDispatcherQueueController(DispatcherQueueOptions options, out IntPtr dispatcherQueueController);

        public static DispatcherQueueController CreateDispatcherQueueControllerForCurrentThread()
        {
            var options = new DispatcherQueueOptions
            {
                dwSize = Marshal.SizeOf<DispatcherQueueOptions>(),
                threadType = DISPATCHERQUEUE_THREAD_TYPE.DQTYPE_THREAD_CURRENT,
                apartmentType = DISPATCHERQUEUE_THREAD_APARTMENTTYPE.DQTAT_COM_NONE
            };

            DispatcherQueueController controller = null;
            uint hr = CreateDispatcherQueueController(options, out IntPtr controllerPointer);
            if (hr == 0)
            {
                controller = DispatcherQueueController.FromAbi(controllerPointer);
                Marshal.Release(controllerPointer);
            }

            return controller;
        }
    }
}
