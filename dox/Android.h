/*! \page android Integrate MadButterfly with Android
 *
 * I think almost all technical guys know Android, the mobile OS
 * project from Google.  Once I know Android, my first idea is it is
 * cool if Android programmers can be powered by MadButterfly.
 * MadButterfly is very feasible for a lof of programs running on
 * devices that Android targets for.  This document is about how the
 * MadButterfly been integrated with Android and power Android
 * applications.
 *
 * \section android_jni Android Native Code
 *
 * Android applications are actually Java code running on Dalvik VM.
 * Although, Java bytecode was translated into bytecode of Dalvik.
 * The interface of bytecode running in VM and native code is JNI,
 * just like Java.  So, we must implement a set of API on JNI as the
 * channel for communication between Android applications and
 * MadButterfly.  Following image is how the MadButterfly integrated
 * with the Android.
 *
 * \image html Android-int.png
 *
 * MB JNI is JNI part of MadButterfly for interaction between
 * Madbufferfly and Android applications.  Android applications draw
 * everything on Canvases (backed by a surface).  A canvas is actually
 * bridge to a SkCanvas provided by Skia.  So, MadButterfly can draw
 * on the SkCanvas for drawing on screen.
 *
 * The idea is to make MadButterfly as a View of Android UI.
 * SurfaceView is extended as MBView; the type of view for
 * MadButterfly.  The SurfaceView own a surface.  The surface is
 * passed to MadButterfly as a SurfaceHolder.  MB JNI get Canvas from
 * the SurfaceHolder, and get SkCanvas of Canvas in turn.  With
 * SkCanvas, MadButterfly can draw on the screen for the MBView
 * object.
 *
 * \section android_mem Memory Management for Android JNI Interface
 *
 * Some memory allocated and returned by MadButterfly should be
 * managed by the application.  A memory block should not be freed
 * until it is no more used by MadButterfly engine.  It means Android
 * applications reponsible for managing memory blocks.  To simplify
 * implmentation of MB JNI, MB JNI always return the address of an
 * allocated objects as a Java integer.  The Java part should take
 * care about these addresses, and call proper functions for freeing
 * objects specified by addresses.
 *
 * A Java side framework should be designed to easy the work of
 * Android application programmers.  The framework should take care
 * the tracing life-cycle of MadButterfly objects.  The namespace of
 * the framework should be 'org.madbutterfly'.
 *
 * \section android_jni _jni
 * 
 * org.madbutterfly._jni class is the placeholder to collect JNI APIs
 * for all MadButterfly functions.  All methods of _jni are static.  A
 * method usually maps to a MadButterfly function.  For example,
 * rdman_coord_new() mapes to a MadButterfly function.
 *
 * The object returned by a MadButterfly function, for
 * ex. rdman_coord_new(), is casted to a int, the address of the
 * object.  So, type of an argument or return value of an object is
 * declared as a int.  For example,
 * \code
 * class _jni {
 *   native static int rdman_coord_new(void);
 * }
 * \endcode
 * Java code should keep these integers carefully for maintaining
 * life-cycle of objects.
 *
 * MadButterfly provides only initial function for some objects,
 * application should allocate memory for these function by them-self.
 * For these function, a new JNI interface are used instead of
 * initiali function.  The new JNI interface is responsible for
 * allocating memory and call initial function.  For example,
 * redraw_man_new() is defined as a JNI interface instead of
 * redraw_man_init().
 * \code
 *   native static int redraw_man_new(int cr, int backend);
 * \endcode
 * First argument, rdman, of redraw_man_init() is replaced by a int
 * returned value.  cr and backend, mbe_t type by MadButterfly, are
 * now int type by JNI interface.
 *
 * \section android_java_mb Java Likes MadButterfly
 *
 * To manage life-cycle for MadButterfly objects is not Java likes.
 * To provie a Java style interface for MadButterfly, a framework to
 * hide underlying life-cycle management is requried.  Life-cycle is
 * one major responsibility of the framework, it should keep integers
 * of addresses of objects and free the associated resources with
 * correct JNI methods and at right time.
 *
 * For example, org.madbutterfly.redraw_man is the respective Java
 * class of redraw_man_t of MadButterfly engine.  redraw_man has a
 * private member variable redraw_man._rdman_addr.  It is the address
 * of the resptive redraw_man_t object of MadButterfly.  redraw_man
 * would call _jni.redraw_man_destroy() to release associated
 * resources and free the memory before it is recycled.  So,
 * redraw_man.finalize() is defined for releasing resources.  It would
 * be called before the redraw_man object being recycled.
 *
 */
