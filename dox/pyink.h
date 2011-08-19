/*! \page pyink_arch Architecture of pyink
 * 
 * pyink/ sub-directory is an implementation base on pybind.  pybind
 * is a binding for Python that exploses functions of Inkscape to
 * Python.  You can access internal data and functions provided by
 * Inkscape, and extend functions of Inkscape platform.
 *
 * pyink.py includes entry points to initialize Python code when
 * Inkscape is booting.  With pybind, Inkscape will try to search and
 * load pyink.py module from directories given by PYTHONPATH
 * environment variable.  pyink.pyink_start() would be called by
 * Inkscape to give Python code a chance to initialize itself and
 * register handlers for hooks provided by pybind.  Pybind defines
 * some hooks that will be tirggered for various events.  By register
 * handlers for hooks, Python code can intervent behavior of Inkscape
 * to modify its heavior.
 *
 * Python code defined by pyink.py can also modify UI of Inkscape by
 * manipulate component tree of GTK.  In our implementation, it
 * invokes PyGTK to inspect and modify GUI of Inkscape.  For example,
 * add list views or/and menu items.
 *
 * \section arch_pyink_mb Aricthrue of pyink module provied by MadButterfly
 *
 * The purpose of pyink provided by MadButterfly is for making GUI of
 * other applications.  It includes several major parts.
 *  - domview
 *  - domview_ui
 *  - frameline
 */
