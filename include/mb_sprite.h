#ifndef __MB_SPRITE_H_
#define __MB_SPRITE_H_

#include "mb_types.h"
#include "mb_redraw_man.h"

/*! \defgroup mb_sprite  Implement sprites for animation.
 * @{
 */
/*! \brief A sprite is a set of graphics that being an object in animation.
 *
 * A sprite include graphics comprise an object.  For example, a tank, in
 * example tank, is comprised a set of graphics that is represented as a
 * sprite.
 */
struct _mb_sprite {
    void (*free)(mb_sprite_t *);
    mb_obj_t *(*get_obj_with_name)(mb_sprite_t *sprite, const char *id);
    /*! Return non-zero for error. */
    int (*goto_scene)(mb_sprite_t *sprite, int scene_no);
};

#define MB_SPRITE_FREE(sprite) ((mb_sprite_t *)(sprite))->free(sprite)
#define MB_SPRITE_GET_OBJ(sprite, name)					\
    ((mb_sprite_t *)(sprite))->get_obj_with_name((mb_sprite_t *)(sprite), \
						 (name))
#define MB_SPRITE_GOTO_SCENE(sprite, scene_no)				\
    ((mb_sprite_t *)(sprite))->goto_scene((mb_sprite_t *)(sprite), scene_no)


/*! \brief Load sprite dymanicly from the shared object module.
 *
 * The search path can be changed by sprite_set_search_path. The name
 * can have a relative path in the front of it.
 * This function will search the object in the current working directory
 * and then search the system search path.
 */
extern mb_sprite_t *sprite_load(const char *name, redraw_man_t *rdman,
				coord_t *root);

/*! \brief Set the search path of dymanic object loading.
 *
 */
extern void sprite_set_search_path(const char *path);

/*! \defgroup mb_sprite_lsym Sprite with linear symbol table.
 * @{
 */
struct _mb_sprite_lsym_entry {
    const char *sym;
    const int offset;
};
typedef struct _mb_sprite_lsym_entry mb_sprite_lsym_entry_t;

/*! \brief A sub-type of mb_sprite_t with linear symbol table.
 *
 * This type of sprite search symbols with linear/or binary searching.
 */
struct _mb_sprite_lsym {
    mb_sprite_t sprite;
    int num_entries;
    mb_sprite_lsym_entry_t *entries;
};
typedef struct _mb_sprite_lsym mb_sprite_lsym_t;
/* @} */
/* @} */

#endif /* __MB_SPRITE_H_ */

