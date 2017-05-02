namespace System.Runtime.CompilerServices {
    [FriendAccessAllowed]
    internal static class JitHelpers
    {
        // The special dll name to be used for DllImport of QCalls
        internal const string QCall = "QCall";

	    static internal T UnsafeCast<T>(Object o) where T : class
	    {
		    return Array.UnsafeMov<object, T> (o);
	    }

	    static internal int UnsafeEnumCast<T>(T val) where T : struct
	    {
		    return Array.UnsafeMov<T, int> (val);
	    }

	    static internal long UnsafeEnumCastLong<T>(T val) where T : struct
	    {
		    return Array.UnsafeMov<T, long> (val);
	    }

	    static internal IntPtr UnsafeCastToStackPointer<T>(ref T val)
	    {
		    return Array.UnsafeMov<T, IntPtr> (val);

	    }
    }

}