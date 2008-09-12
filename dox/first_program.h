/*! \page first_program Your First MadButterfly Program.
 *
 * MadButterfly use SVG as a media to adapt gap between GUI designers
 * and application programmmers.  Designers export their works with SVG
 * format. Then some ones, designers or programmers, assign IDs to SVG tags
 * that will be manipulated by application.  For example, you have a menu
 * that should be hidden at beginning.  When user hit a button, the menu
 * should be showed.  So, we assign an ID to the group or other tags that
 * contain all elements in menu.  The ID is the name of a object that
 * application manipulate, hide and show it.
 *
 * After assigning IDs to tags, the file is translated by svg2code.py.
 * Outputs of svg2code.py are M4 macro files.  Conventional, foo.svg is
 * translated a M4 file as foo.mb, using .mb as extension of file name.
 * Macro files are translate to *.c and *.h files to link with application
 * programs.
 *
 * For example, to translate foo.svg with steps
 * - $(PREFIX)/bin/svg2code.py foo.svg foo.mb
 * - m4 -I $(PREFIX)/share/mb mb_c_source.m4 foo.mb > foo.c
 * - m4 -I $(PREFIX)/share/mb mb_c_header.m4 foo.mb > foo.h
 *
 * foo.h declares a structure, named 'foo' and two functions,
 * foo_new() and foo_free(). An instance of 'foo' holds all objects for
 * foo.svg.  One object, with specified ID as name, for each tag.  If you
 * don't assign one, a random one is picked.  foo_new() is invoked to create
 * and initialize a 'foo' instance.  An instance is released by calling
 * foo_free().
 *
 * - \subpage svg2code_ex
 */
