/*! \page archi Internal Architecture
 *
 * \image html core.png
 * Above graphic is relationship among major objects of MadButterfly.
 * \ref redraw_man_t, redraw manager, is supposed to manage graphic elements,
 * shapes, and decide when and who to redraw on the output screen.
 *
 * \ref shape_t is base-type for all graphic objects.  It can be a path,
 * a text, a ellipse, ..., or an image.  When the states or attributes of
 * an graphic
 * object changed, managing redraw_man_t, the one that manages the graphic
 * object, will be notified.  The redraw_man_t will schedule a redrawing
 * for every
 * dirty objects, changed ones, and relative objects.  The relative objects
 * are objects effected by changed ones.  For example, part of a relative
 * object may be hidden before changing, but it is re-explored after chaning.
 * The explored part of the object must be redrawed.
 *
 * The shape of shape objects are vary from type to type.  To simplize
 * the problem, we use a rectangle to model the area occupied by an object.
 * \ref geo_t is the one used to model the rectangle.  Shape objects should
 * update associated \ref geo_t objects to reflect changing of them-self.
 *
 * \ref coord_t is used to model transformation in coordinate system.
 * A \ref coord_t is actually an transform matrix with additional
 * informations.  \ref coord_t
 * objects are organized as a tree according hierachy of group tags in
 * a SVG file.  coord_t has a member variable, called before_pmem, that
 * is relative position in parent coord.  \ref coord_t::before_pmem stands for
 * "before parent member", it is an index to the list of member shapes
 * of parent coord.  Members of a coord is shape tags they are children of
 * the "g" tag of the coord corresponding.  \ref coord_t::before_pmem means
 * "g" of the coord is before the indexed member of parent.
 *
 * \image html geom_relation.png
 *
 * It is possible to have more than one coord with the same
 * \ref coord_t::before_pmem value and as children of a coord.  It means
 * all children with the same coord_t::before_pmem value are all before
 * nth member, after (n-1)th member, and they are in order of them-self
 * in the children list.
 *
 * Colors or patterns used to stroke or fill shapes are described
 * by \ref paint_t .  Every shape object has a paint, but a paint object
 * can be used by multiple shape objects.  Once a paint is changed, all
 * objects that associate with the paint are dirty.  They will be scheduled
 * by associated \ref redraw_man_t object to update graphics on
 * output screen.
 *
 */
