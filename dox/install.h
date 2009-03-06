/*! \page install How to Build and Install MadButterfly?
 *
 * Prerequisite
 * - autotools
 *  - autoconf
 *  - automake
 *  - libtools
 * - install Cairo
 * - install Pango
 *
 * Get source
 * - hg clone http://hg.assembla.com/MadButterfly MadButterfly
 *
 * Build and Install
 * - cd MadButtfly
 * - ./autogen.sh
 * - ./configure
 * - make
 * - make install
 *
 * Examples
 * - cd examples/calculator
 * - make
 * - ./calc
 *
 * - cd examples/svg2code_ex
 * - make
 * - ./ex1
 *
 * - cd examples/tank
 * - make
 * - ./tank
 *
 * You can make examples with following command,
 * \code
 * make PREFIX=/path/to/some/where/
 * \endcode
 * if MadButterfly was not installed in default path, with different prefix.
 *
 * You can install MadButterfly somewhere, other than /usr/local/.
 * For example
 * \code
 * ./configure --prefix=${PWD}/dest
 * \endcode
 */
