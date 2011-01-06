// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
/*! \defgroup xnjsmb MadButterfly JS binding for V8 and nodejs
 *
 * This implementation of JS binding heavily relies on
 * tools/gen_v8_binding.m4, a set of m4 macros.  We defines binding in
 * nodejs/*.m4 files and generate respective code, nodejs/*-inc.h,
 * with macros defined in tools/gen_v8_binding.m4.  *-inc.h files are
 * included by respective *.cc files they implement functions needed
 * by generated code.
 */
