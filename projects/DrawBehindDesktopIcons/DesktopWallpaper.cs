namespace DrawBehindDesktopIcons
{
    using System;
    using System.Runtime.InteropServices;

    [ComImport]
    [Guid("C2CF3110-460E-4fc1-B9D0-8A1C0C9CC4BD")]
    internal class DesktopWallpaper
    {
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-idesktopwallpaper
    [ComImport]
    [Guid("B92B56A9-8B55-4E14-9A89-0199BBB6F93B")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IDesktopWallpaper
    {
        // https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/metadata/imetadataemit-definemethod-method#slots-in-the-v-table
        // https://github.com/dotnet/roslyn/blob/main/src/Compilers/Core/Portable/MetadataReader/ModuleExtensions.cs
        void _VtblGap1_15(); // Skip all the other stuff

        void Enable(bool enable);
    }
}
