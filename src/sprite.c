#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_graph_engine.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include "mb_types.h"
#include "mb_shapes.h"
#include "mb_tools.h"
#include "mb_redraw_man.h"
#include "mb_observer.h"
#include "mb_prop.h"

#define ASSERT(x)
#define OK 0
#define ERR 1

static char *sprite_search_path = NULL;

static char *sprite_search_so(const char *name) {
    struct stat st;
    int fsz;
    char *fullname;
    int r;
    
    if(sprite_search_path == NULL)
	sprite_search_path = strdup("/usr/share/madbutterffly");
    
    fsz = strlen(sprite_search_path) + strlen(name) + 5;
    fullname = (char *)malloc(fsz);

    snprintf(fullname, fsz, "%s/%s.so", sprite_search_path, name);
    r = stat(fullname, &st);
    if(r != 0) {
	free(fullname);
	return NULL;
    }

    return fullname;
}

void sprite_set_search_path(char *path) {
    int sz;
    
    if (sprite_search_path)
	free(sprite_search_path);
    
    sprite_search_path = strdup(path);
    
    sz = strlen(sprite_search_path);
    if(sprite_search_path[sz - 1] == '/')
	sprite_search_path[sz - 1] = 0;
}

mb_sprite_t *
sprite_load(const char *name, redraw_man_t *rdman, coord_t *root) {
    char cnstr_name[256];
    char *so_path;
    const char *bname;
    void *handle;
    mb_sprite_t *(*cnstr)(redraw_man_t *, coord_t *);
    mb_sprite_t *obj;
    int r;
    
    so_path = sprite_search_so(name);
    if(so_path == NULL)
	return NULL;
    
    handle = dlopen(so_path, RTLD_LAZY);
    free(so_path);
    if (handle == NULL)
	return NULL;
    
    bname = strrchr(name, '/');
    if(bname != NULL && strlen(bname) > 250)
	return NULL;
    
    if(bname == NULL)
	bname = name;
    else
	bname++;
    
    snprintf(cnstr_name, sizeof(cnstr_name), "%s_new", bname);
    cnstr = dlsym(handle, cnstr_name);
    if (cnstr == NULL)
	return NULL;
    
    obj = cnstr(rdman, root);
    
    return obj;
}

/* vim: set ts=4 */
