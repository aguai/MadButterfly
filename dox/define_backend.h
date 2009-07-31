/*! \page backend How to Define a Backend
 *
 * A backend is factory to initialize environment to make
 * MadBufferfly available.  A backend should provide resources
 * needed by MadButterfly, for example, to provide a surface
 * that will show everything drawing on it.  It also translate and
 * relay input events, mouse or keyboard, to MadButterfly.
 * The tasks that a backend should do are listed following,
 *  - to prepare a backend surface,
 *  - to prepare a front surface,
 *  - to translate and relay input events to MadButterfly,
 *  - to handle a timer, and relay timeout events to MadButterfly.
 * 
 * The output device surface for X Window is a surface return by
 * cairo_xlib_surface_create().  MadButterfly will copy everything
 * from front surface to backend surface to show graphy to user.
 * The copying is to avoid user find slowly redrawing.  The latency
 * between X client and server can be large.  For this situation,
 * we need a font surface as a buffer drawing, and copy image from
 * front surface to backend surface after completion of a series
 * of drawing.  A front surface can be an image surface for this
 * situation.
 *
 * The input events of X Window should be translated to raw events of
 * MadButterfly and sent them to rdman through notify_coord_or_shape()
 * function.
 *
 * \see X_supp.c
 */
