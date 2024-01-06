EGL_ENTRY(EGLDisplay, eglGetDisplay, NativeDisplayType)
EGL_ENTRY(EGLBoolean, eglInitialize, EGLDisplay, EGLint*, EGLint*)
EGL_ENTRY(EGLBoolean, eglTerminate, EGLDisplay)
EGL_ENTRY(EGLBoolean, eglGetConfigs, EGLDisplay, EGLConfig*, EGLint, EGLint*)
EGL_ENTRY(EGLBoolean, eglChooseConfig, EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *)

EGL_ENTRY(EGLBoolean, eglGetConfigAttrib, EGLDisplay, EGLConfig, EGLint, EGLint *)
EGL_ENTRY(EGLSurface, eglCreateWindowSurface, EGLDisplay, EGLConfig, NativeWindowType, const EGLint *)
EGL_ENTRY(EGLSurface, eglCreatePixmapSurface, EGLDisplay, EGLConfig, NativePixmapType, const EGLint *)
EGL_ENTRY(EGLSurface, eglCreatePbufferSurface,  EGLDisplay, EGLConfig, const EGLint *)
EGL_ENTRY(EGLBoolean, eglDestroySurface, EGLDisplay, EGLSurface)
EGL_ENTRY(EGLBoolean, eglQuerySurface,  EGLDisplay, EGLSurface, EGLint, EGLint *)
EGL_ENTRY(EGLContext, eglCreateContext, EGLDisplay, EGLConfig, EGLContext, const EGLint *)
EGL_ENTRY(EGLBoolean, eglDestroyContext, EGLDisplay, EGLContext)
EGL_ENTRY(EGLBoolean, eglMakeCurrent, EGLDisplay, EGLSurface, EGLSurface, EGLContext)
EGL_ENTRY(EGLContext, eglGetCurrentContext, void)
EGL_ENTRY(EGLSurface, eglGetCurrentSurface, EGLint)
EGL_ENTRY(EGLDisplay, eglGetCurrentDisplay, void)
EGL_ENTRY(EGLBoolean, eglQueryContext,  EGLDisplay, EGLContext, EGLint, EGLint *)
EGL_ENTRY(EGLBoolean, eglWaitGL, void)
EGL_ENTRY(EGLBoolean, eglWaitNative, EGLint)
EGL_ENTRY(EGLBoolean, eglSwapBuffers, EGLDisplay, EGLSurface)
EGL_ENTRY(EGLBoolean, eglCopyBuffers, EGLDisplay, EGLSurface, NativePixmapType)
EGL_ENTRY(EGLint, eglGetError, void)
EGL_ENTRY(const char*, eglQueryString, EGLDisplay, EGLint)
EGL_ENTRY(__eglMustCastToProperFunctionPointerType, eglGetProcAddress, const char *)

/* EGL 1.1 */

EGL_ENTRY(EGLBoolean, eglSurfaceAttrib, EGLDisplay, EGLSurface, EGLint, EGLint)
EGL_ENTRY(EGLBoolean, eglBindTexImage, EGLDisplay, EGLSurface, EGLint)
EGL_ENTRY(EGLBoolean, eglReleaseTexImage, EGLDisplay, EGLSurface, EGLint)
EGL_ENTRY(EGLBoolean, eglSwapInterval, EGLDisplay, EGLint)

/* EGL 1.2 */

EGL_ENTRY(EGLBoolean, eglBindAPI, EGLenum)
EGL_ENTRY(EGLenum, eglQueryAPI, void)
EGL_ENTRY(EGLBoolean, eglWaitClient, void)
EGL_ENTRY(EGLBoolean, eglReleaseThread, void)
EGL_ENTRY(EGLSurface, eglCreatePbufferFromClientBuffer, EGLDisplay, EGLenum, EGLClientBuffer, EGLConfig, const EGLint *)

/* EGL 1.3 */

/* EGL 1.4 */

/* EGL 1.5 */
EGL_ENTRY(EGLImage, eglCreateImage, EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib *)
EGL_ENTRY(EGLBoolean, eglDestroyImage, EGLDisplay, EGLImage)
EGL_ENTRY(EGLDisplay, eglGetPlatformDisplay, EGLenum, void *, const EGLAttrib *)
EGL_ENTRY(EGLSurface, eglCreatePlatformWindowSurface, EGLDisplay, EGLConfig, void *, const EGLAttrib *)
EGL_ENTRY(EGLSurface, eglCreatePlatformPixmapSurface, EGLDisplay, EGLConfig, void *, const EGLAttrib *)
EGL_ENTRY(EGLSyncKHR, eglCreateSync, EGLDisplay, EGLenum, const EGLAttrib *)
EGL_ENTRY(EGLBoolean, eglDestroySync, EGLDisplay, EGLSync)
EGL_ENTRY(EGLint, eglClientWaitSync, EGLDisplay, EGLSync, EGLint, EGLTimeKHR)
EGL_ENTRY(EGLBoolean, eglGetSyncAttrib, EGLDisplay, EGLSync, EGLint, EGLAttrib *)
EGL_ENTRY(EGLBoolean, eglWaitSync, EGLDisplay, EGLSync, EGLint)

/* EGL_EGLEXT_VERSION 3 */

EGL_ENTRY(EGLBoolean,  eglLockSurfaceKHR,   EGLDisplay, EGLSurface, const EGLint *)
EGL_ENTRY(EGLBoolean,  eglUnlockSurfaceKHR, EGLDisplay, EGLSurface)
EGL_ENTRY(EGLImageKHR, eglCreateImageKHR,   EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *)
EGL_ENTRY(EGLBoolean,  eglDestroyImageKHR,  EGLDisplay, EGLImageKHR)

/* EGL_EGLEXT_VERSION 5 */

EGL_ENTRY(EGLSyncKHR,   eglCreateSyncKHR,       EGLDisplay, EGLenum, const EGLint *)
EGL_ENTRY(EGLBoolean,   eglDestroySyncKHR,      EGLDisplay, EGLSyncKHR)
EGL_ENTRY(EGLint,       eglClientWaitSyncKHR,   EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR)
EGL_ENTRY(EGLBoolean,   eglSignalSyncKHR,       EGLDisplay, EGLSyncKHR, EGLenum)
EGL_ENTRY(EGLBoolean,   eglGetSyncAttribKHR,    EGLDisplay, EGLSyncKHR, EGLint, EGLint *)

/* EGL_EGLEXT_VERSION 15 */

EGL_ENTRY(EGLStreamKHR, eglCreateStreamKHR,     EGLDisplay, const EGLint *)
EGL_ENTRY(EGLBoolean,   eglDestroyStreamKHR,    EGLDisplay, EGLStreamKHR)
EGL_ENTRY(EGLBoolean,   eglStreamAttribKHR,     EGLDisplay, EGLStreamKHR, EGLenum, EGLint)
EGL_ENTRY(EGLBoolean,   eglQueryStreamKHR,      EGLDisplay, EGLStreamKHR, EGLenum, EGLint *)
EGL_ENTRY(EGLBoolean,   eglQueryStreamu64KHR,   EGLDisplay, EGLStreamKHR, EGLenum, EGLuint64KHR *)
EGL_ENTRY(EGLBoolean,   eglStreamConsumerGLTextureExternalKHR,  EGLDisplay, EGLStreamKHR)
EGL_ENTRY(EGLBoolean,   eglStreamConsumerAcquireKHR,            EGLDisplay, EGLStreamKHR)
EGL_ENTRY(EGLBoolean,   eglStreamConsumerReleaseKHR,            EGLDisplay, EGLStreamKHR)
EGL_ENTRY(EGLSurface,   eglCreateStreamProducerSurfaceKHR,      EGLDisplay, EGLConfig, EGLStreamKHR, const EGLint *)
EGL_ENTRY(EGLBoolean,   eglQueryStreamTimeKHR,  EGLDisplay, EGLStreamKHR, EGLenum, EGLTimeKHR*)
EGL_ENTRY(EGLNativeFileDescriptorKHR,   eglGetStreamFileDescriptorKHR,          EGLDisplay, EGLStreamKHR)
EGL_ENTRY(EGLStreamKHR, eglCreateStreamFromFileDescriptorKHR,   EGLDisplay, EGLNativeFileDescriptorKHR)
EGL_ENTRY(EGLint,       eglWaitSyncKHR,         EGLDisplay, EGLSyncKHR, EGLint)

/* EGL_EGLEXT_VERSION 20170627 */
EGL_ENTRY(EGLSurface, eglCreatePlatformWindowSurfaceEXT, EGLDisplay, EGLConfig, void *, const EGLint *)
EGL_ENTRY(EGLSurface, eglCreatePlatformPixmapSurfaceEXT, EGLDisplay, EGLConfig, void *, const EGLint *)

/* ANDROID extensions */

/* Partial update extensions */

EGL_ENTRY(EGLBoolean, eglSwapBuffersWithDamageKHR, EGLDisplay, EGLSurface, EGLint *, EGLint)
EGL_ENTRY(EGLBoolean, eglSetDamageRegionKHR, EGLDisplay, EGLSurface, EGLint *, EGLint)
