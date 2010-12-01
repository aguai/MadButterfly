// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MBOBJ_H
#define __MBOBJ_H
#include "mb_shapes.h"
#include "mb_shapes.h"
/*! \brief Change the location of the object.
 *
 * The location of the object will be relocated to the new position. This function works for group(coord_t)
 * and text shape (shape_text_t). For others, this command will be ignored since we don't know the original
 * point of a path.
 */
void mb_obj_set_pos(mb_obj_t *obj, co_aix x, co_aix y);

/*! \brief Get the position of the object.
 *
 *  Return the position of the object. This works for group and text only. For others, (0,0) will be returned.
 */
void mb_obj_get_pos(mb_obj_t *obj, co_aix *x, co_aix *y);

/*! \brief set the width of the object.
 *
 */
void mb_obj_set_scalex(mb_obj_t *obj, int scale);

/*! \brief return the scale of width.
 */
int mb_obj_get_scalex(mb_obj_t *obj);

/*! \brief set the scale of height.
 */
void mb_obj_set_scaley(mb_obj_t *obj, int scale);
/*! \brief return the scale of height
 *
 */
int mb_obj_get_scaley(mb_obj_t *obj);


/*! \brief Change the rotation degree.
 *
 */
void mb_obj_set_rotation(mb_obj_t *obj, int degree);

/*! \brief Return the rotation degree of an object.
 *
 */

int mb_obj_get_rotation(mb_obj_t *obj);

/*! \brief set the textformat of the texts inside a group.
 *
 *  If the obj is a group, we will search the first text element inside it. If it is a shape_t, it will be applied to it directly.
 *
 */
void mb_obj_set_textstyle(mb_obj_t *obj, mb_textstyle_t *format, int begin, int end);

/*! \brief return the text format style between 'begin' and 'end'.
 *
 */
void mb_obj_get_textstyle(mb_obj_t *, mb_textstyle_t *format,int begin,int end);

/*! \brief Change the characters of a text field.
 *  Change the content of a text field. If the obj is a group, we will search for the first text field inside it as the target.
 */
void mb_obj_set_text(mb_obj_t *obj, const char *text);


/*! \brief Get the content of a text field.
 *
 *   The 'text' is the data buffer and the 'size' is the size of the buffer.
 */
void mb_obj_get_text(mb_obj_t *obj, char *text, int size);

#endif

