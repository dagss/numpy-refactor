﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NumpyDotNet
{
    /// <summary>
    /// Base class for wrapped core objects.
    /// </summary>
    public class Wrapper : IDisposable
    {
        internal Wrapper() {
        }

        ~Wrapper() {
            Dispose(false);
        }

        public void Dispose() {
            Dispose(true);
        }

        protected void Dispose(bool disposing) {
            if (core != IntPtr.Zero) {
                lock (this) {
                    // If the core reference count is non-zero then
                    // we can't safely dealloc and we just ignore
                    // the dispose call.
                    if (NpyCoreApi.GetRefcnt(core) == IntPtr.Zero) {
                        IntPtr tmp = core;
                        core = IntPtr.Zero;
                        NpyCoreApi.Dealloc(tmp);
                        if (disposing) {
                            GC.SuppressFinalize(this);
                        }
                    }
                }
            }
        }


        /// <summary>
        /// A pointer to the wrapped core object.
        /// </summary>
        protected IntPtr core;
    }
}
