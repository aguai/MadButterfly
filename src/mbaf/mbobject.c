#include "mb_types.h"
#include "mb_obj.h"


void mb_obj_set_pos(mb_obj_t *obj, co_aix x, co_aix y)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	    coord_x(((coord_t *) obj)) = x;
	    coord_y(((coord_t *) obj)) = y;
    } else if (MBO_TYPE(obj) == MBO_TEXT) {
	    sh_text_set_pos((shape_t *) obj, x, y);
    } else {
	    return;
    }
    
}

void mb_obj_get_pos(mb_obj_t *obj, co_aix *x, co_aix *y)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	    *x = coord_x((coord_t *) obj);
	    *y = coord_y((coord_t *) obj);
    } else if (MBO_TYPE(obj) == MBO_TEXT) {
	    sh_text_get_pos((shape_t *) obj, x, y);
    } else {
	    return;
    }
    
}

void mb_obj_set_text(mb_obj_t *obj, const char *text)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
        geo_t *geo;
        shape_t *shape;
	coord_t *g = (coord_t *) obj;

        FOR_COORD_MEMBERS(g, geo) {
            shape = geo_get_shape(geo);
            if(shape->obj.obj_type == MBO_TEXT) {
		sh_text_set_text(shape, text);
		return;
            }
        }
    } else if (MBO_TYPE(obj) == MBO_TEXT) {
	    sh_text_set_text((shape_t *) obj,text);
    } else {
	    return;
    }
    
}


void mb_obj_get_text(mb_obj_t *obj, char *text,int size)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
        geo_t *geo;
        shape_t *shape;
	coord_t *g = (coord_t *) obj;

        FOR_COORD_MEMBERS(g, geo) {
            shape = geo_get_shape(geo);
            if(shape->obj.obj_type == MBO_TEXT) {
		sh_text_get_text(shape, text,size);
		return;
            }
        }
    } else if (MBO_TYPE(obj) == MBO_TEXT) {
	    sh_text_get_text((shape_t *) obj,text,size);
    } else {
	    *text = 0;
	    return;
    }
}

void mb_obj_set_scalex(mb_obj_t *obj,int scale)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	coord_set_scalex((coord_t *) obj, scale);
    } else {
    }
}

int mb_obj_get_scalex(mb_obj_t *obj)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	return coord_scalex((coord_t *) obj);
    } else {
	return 100;
    }
}

void mb_obj_set_scaley(mb_obj_t *obj,int scale)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	coord_set_scaley((coord_t *) obj, scale);
    } else {
    }
}
int mb_obj_get_scaley(mb_obj_t *obj)
{
    if (MBO_TYPE(obj) == MBO_COORD) {
	return coord_scaley((coord_t *) obj);
    } else {
	return 100;
    }
}

void mb_obj_set_rotation(mb_obj_t *obj, int degree)
{
    printf("%s is not implemented yet\n",__FUNCTION__);
}

int mb_obj_get_rotation(mb_obj_t *obj)
{
    printf("%s is not implemented yet\n",__FUNCTION__);
}



void mb_obj_set_color(mb_obj_t *obj, int color)
{
    printf("%s is not implemented yet\n",__FUNCTION__);
}


int mb_obj_get_color(mb_obj_t *obj)
{
    printf("%s is not implemented yet\n",__FUNCTION__);
    return 0;
}

