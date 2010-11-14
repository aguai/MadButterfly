// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
/*! \file
 * \brief Define types for sprite shared objects.
 *
 * A sprite shared object includes definitions of multiple sprite factories.
 * Sprite factories are used to create sprites for MadButterfly.
 *
 * \see http://en.wikipedia.org/wiki/Sprite_(computer_graphics)
 */
#ifndef __MB_SO_H_
#define __MB_SO_H_

/*! \brief Name of the variable that define contents in a shared object. */
#define MB_SPRITE_SO_SYM "mb_sprite_so_def"

typedef struct _mb_sprite_so mb_sprite_so_t;
typedef struct _sprite_factory sprite_factory_t;


/*! \brief Define content of a sprite shared object.
 *
 * The type of symbol with name, defined by \ref MB_SPRITE_SO_SYM, in
 * a sprite shared object.  It define content of a sprite object, a.k.a.
 * an array of sprite factories (\ref sprite_factory_t).
 */
struct _mb_sprite_so {
    const char *soname;
    int num_factories;
    sprite_factory_t *factories;
};

/*! \brief Define a factory to create sprites.
 */
struct _sprite_factory {
    const char *name;
    sprite_t *(*new)(coord_t*);
};

#endif /* __MB_SO_H_ */
