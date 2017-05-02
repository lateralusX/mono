using System.Security;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace  System.StubHelpers {

    [FriendAccessAllowed]
    internal static class EventArgsMarshaler
    {
        [SecurityCritical]
        [FriendAccessAllowed]
        static internal IntPtr CreateNativeNCCEventArgsInstance(int action, object newItems, object oldItems, int newIndex, int oldIndex)
        {
     		throw new NotImplementedException ();
        }

        [SecurityCritical]
        [FriendAccessAllowed]
        [DllImport(JitHelpers.QCall), SuppressUnmanagedCodeSecurity]
        static extern internal IntPtr CreateNativePCEventArgsInstance([MarshalAs(UnmanagedType.HString)]string name);

        [SecurityCritical]
        [DllImport(JitHelpers.QCall), SuppressUnmanagedCodeSecurity]
        static extern internal IntPtr CreateNativeNCCEventArgsInstanceHelper(int action, IntPtr newItem, IntPtr oldItem, int newIndex, int oldIndex);
    }

    [FriendAccessAllowed]
    internal static class InterfaceMarshaler
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern IntPtr ConvertToNative(object objSrc, IntPtr itfMT, IntPtr classMT, int flags);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern object ConvertToManaged(IntPtr pUnk, IntPtr itfMT, IntPtr classMT, int flags);

        [SecurityCritical]
        [DllImport(JitHelpers.QCall), SuppressUnmanagedCodeSecurity]
        static internal extern void ClearNative(IntPtr pUnk);

        [FriendAccessAllowed]
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern object ConvertToManagedWithoutUnboxing(IntPtr pNative);
    } 

}