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
 */
