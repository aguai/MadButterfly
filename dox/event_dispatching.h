/*! \page event_dispatching Event Dispatching Mechanism in MadButterfly
 *
 * by wycc
 *
 * \section evt_intro Introduction
 *
 * Event dispatching is an important job for the GUI system. Usually,
 * we need to handle the following events in the GUI
 *
 * - Mouse/pointer events
 * - Key event
 * - timer event
 * - IO event
 * - Repaint event
 *
 * In the rest of this article, I will discuss the event mechanism of
 * the MadButterfly and provide a design of event API.
 *
 * First thing go first. We will start from the most important event -
 * mouse evnets.
 *
 * \section evt_mouse Mouse/point events
 *
 * MadButterfly use observer design pattern to implement the event
 * dispatching. In this pattern, observer and subject are used to
 * handle events. The subject is a specific type of events. For
 * example, mouse events or keyboard events. Each subject can have zero
 * or more observers. When the system send events related to the subject,
 * it will call the subject_notify function of the subject, which will
 * notify every registered observers.
 *
 * In this way, we can decouple the components which send the events and
 * the components which need the events. For example, if we implement the
 * X and GTK backend, both of them will call subject_notify when it
 * receive the mouse events from the X or GTK. The observers don't
 * care whether the notification coming from X or GTK. The X and GTK
 * backend are responsible to translate the X or GTK mouse events into
 * the mouse subject.
 *
 * Therefore, we can use the subject as the astraction layer between
 * MadButterfly clients and the backend.
 *
 * To be more specific, in the current X backend. When it receive a
 * MotionNotify mouse event from the X server, it will send an mouse
 * event to the mouse event subject ob type OBJT_GEO.
 * \code
 * mouse_event.event.type = etype;
 * mouse_event.x = x;
 * mouse_event.y = y;
 * mouse_event.but_state = state;
 * mouse_event.button = button;
 * subject = sh_get_mouse_event_subject(shape);
 * factory = rdman_get_ob_factory(rdman);
 * subject_notify(factory, subject, (event_t *)&mouse_event);
 * \endcode
 *
 * The shape above is determined by the mouse position and the etype
 * is determined the shape under the mouse and the last shape which
 * receive mouse event.
 *
 * - If the mouse is on any shape,
 *   - If the new shape is the same as the last shape, we will send the
 *     EVT_MOUSE_MOVE events to it to notify it the mouse is moving inside it.
 *   - If the new shape is not the same as the last shape, we will send the
 *     EVT_MOUSE_OVER to notify it the mouse is just touch it.
 * - If the current shape is not the same as the last shape, send the
 *    EVT_MOUSE_OUT to the last shape to tell it the mouse have left it.
 * \code
 * mevt = (XMotionEvent *)&evt;
 * x = mevt->x;
 * y = mevt->y;
 * state = get_button_state(mevt->state);
 *
 * shape = find_shape_at_pos(rdman, x, y,
 *                           &in_stroke);
 * if(shape != NULL) {
 *	if(rt->last != shape) {
 *		if(rt->last)
 *			notify_shapes(rdman, rt->last, x, y,
 *                                    EVT_MOUSE_OUT, state, 0);
 *		notify_shapes(rdman, shape, x, y,
 *                            EVT_MOUSE_OVER, state, 0);
 *		rt->last = shape;
 *	} else
 *		notify_shapes(rdman, shape, x, y,
 *			      EVT_MOUSE_MOVE, state, 0);
 * } else {
 *	if(rt->last) {
 *		notify_shapes(rdman, rt->last, x, y,
 *			      EVT_MOUSE_OUT, state, 0);
 *		rt->last = NULL;
 *	}
 * }
 * \endcode
 *
 * Please remember that the subject will relay the the events to all of
 * its parents.
 *
 * PS. Currently, the MadButterfly does not have mechanism to stop
 * propogation based on the result of the observer.
 *
 * \section evt_key Key event
 *
 * The key events is send as subject type OBJT_KB. Each key object has
 * the following fields code: The original raw keycode. sym: The symbol ID
 * of the raw keycode. * event.type: EVT_KB_PRESS or EVT_KB_RELEASE
 *
 * \section evt_timer Timer event
 *
 * The timer use different mechanism. It does not use the observer pattern.
 * Instead, it is registered as separate timer queue. Do we want to change
 * the implementation?
 *
 * \section evt_io IO event
 *
 * Currently, no IO events is available in the MadButterfly yet. W should
 * add the following IO subjects. OBJT_FD: We need to provide some function
 * to generate subject for file description. It will send events when teh
 * file descriptor become available for read/write or error. OBJT_XMLIO:
 * Provides functions similiar to the XMLSocket in actionscript.
 *
 * \section evt_repaint Repaint
 *
 * The repaint event is not sent directly. Instead, the X backend call
 * rdman_redraw_area directly to handle the repaint event. This function
 * will send EVT_RDMAN_REDRAW out.
 *
 * \section evt_summary Summary
 *
 * The observer pattern can be used to be the abstraction layer of the
 * MadButterfly. The current implementation has effectively seperate the
 * backend and the core engine. There is no direct call to the engine
 * from the backend. All messages are deliver to the MadButterfly through
 * the observer.
 */
