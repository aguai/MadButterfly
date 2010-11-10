/*! \page backend How to Define a Backend
 *
 * A backend is factory to initialize environment to make
 * MadBufferfly available.  A backend should provide resources
 * needed by MadButterfly, for example, to provide a surface
 * that will show everything drawing on it.  It also translate and
 * relay input events, mouse or keyboard, to MadButterfly.
 * The tasks that a backend should do are listed following,
 *  - tranlsate and relay mouse events to MadButterfly.
 *  - to provide
 *    - *_MB_new()/*_MB_free function to create and free a runtime.
 *      - prepare a backend surface,
 *      - prepare a front surface,
 *    - *_MB_kbevents() to return a subject, for keyboard
 *      events, associated with a runtime.
 *      - translate and relay keyboard events to MadButterfly,
 *    - *_MB_tman() to return a timer manager associated with
 *      a runtime.
 *      - to handle a timer, and relay timeout events to MadButterfly.
 *    - *_MB_ob_factory() to return an observer factory.
 *    - *_MB_img_ldr() to return an image loader.
 * 
 * \section backend_mb_new_n_free *_MB_new()/*_MB_free()
 *
 * MadButterfly supposes that application programmers may create more
 * than one instance of MadButterfly to draw mutliple windows for an
 * application.  So, we need you, backend developer, to provide a
 * *_MB_new()/*_MB_free() to create/free an instance respective.
 *
 * *_MB_new() should return an *_MB_runtime_t object to keep states of
 * an instance.  The definition of *_MB_runtime_t is up to backend
 * developers.
 *
 * For each *_MB_runtime_t, backend should create a redraw manager,
 * a.k.a rdman, by calling malloc() and redraw_man_init().  For each
 * rdman, you should give it one or two surfaces.  Rdman draws shapes
 * on first one, called 'cr' (should be changed), as an off-screen
 * buffer and copy it to 2nd one, called 'backend' surface, as an
 * on-screen buffer.  For X, the 'backend' should be a window surface.
 * The reason of two surfaces are to prevent user from seeing
 * intermediate result of drawing procedure.  You can also pass a NULL
 * pointer for 2nd surface if you dont care intermediate result, HW
 * accelerator is fast enough, or with HW dobule buffering.
 *
 * \section backend_kbevents Keyboard Events
 *
 * *_MB_kbevents() returns a subject (\see
 * http://en.wikipedia.org/wiki/Observer_pattern).  Applications want
 * to receive keyboard events would register an observer on this
 * subject.  A backend should translate keyboard events for
 * MadButterfly, and notify observers of this subject.
 * Subject-observer had implemented by MadBuffery.
 *
 * \section backend_timer Timer
 *
 * *_MB_tman() should return a timer manager, with type of mb_tman_t.
 * When an application want to get notified at some time later, it
 * would register a callback with the mb_tman_t.  A backend should
 * call mb_tman_handle_timeout() at proper time.  You can call
 * mb_tman_next_timeout() to get the time to call
 * mb_tman_handle_timeout() for next timeout.  For nodejs or other
 * binding that has their-owned timer, you can skip *_MB_tman().  But,
 * C code need this one.
 *
 * \section backend_obfactory Observer Factory
 *
 * *_MB_ob_factory() returns an observer factory, ob_factory_t.  It is
 * reponsible for creation of observers.  Applications call observer
 * to allocate and free observers and subjects.  ob_factory_t is
 * defined in mb_observer.h, there are 5 function pointers in it.  You
 * can use functions of default implementation instead of new ones.
 *
 * \section backend_img_ldr Image Loader
 *
 * *_MB_img_ldr() returns an image loader, mb_img_ldr_t.  A backend
 * developer can use the default implementation.  He can also
 * implement a new one, but it should implement the interface defined
 * by mb_img_ldr_t and mb_img_data_t.
 *
 * \section backend_mouse_events Mouse Events
 *
 * A backend should also translate and relay mouse events to
 * MadButterfly.  The mouse events should be dispatched by notifing
 * observers of the subject of a coord or shape of an rdman.  The
 * backend developers can check handle_single_x_event() in X_supp.c
 * for how to dispatch mouse events.
 *
 * \see X_supp.c
 */
