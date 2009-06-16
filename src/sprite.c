#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "mb_types.h"
#include "mb_shapes.h"
#include "mb_tools.h"
#include "mb_redraw_man.h"
#include "mb_observer.h"
#include "mb_prop.h"

static char *Sprite_Search_Path=NULL;
static int sprite_search_so(char *path, int len, const char *name)
{
	struct stat st;

	if (Sprite_Search_Path == NULL) {
		Sprite_Search_Path = strdup("/usr/share/madbutterffly");
	}

	snprintf(path, len, "./%s.so", name);
	if (stat(path, &st)==0) {
		return 0;
	}
	snprintf(path, len, "%s/%s.so", Sprite_Search_Path, name);
	if (stat(path, &st)==0) {
		return 0;
	} else {
		return -1;
	}
}

void sprite_set_search_path(char *path)
{
	if (Sprite_Search_Path)
		free(Sprite_Search_Path);
	Sprite_Search_Path = strdup(path);
}

mb_sprite_t *sprite_load(const char *name, redraw_man_t *rdman, coord_t *root)
{
	char path[1024];
	const char *s;
	void *handle;
	mb_sprite_t *(*new)(redraw_man_t *, coord_t *);
	mb_sprite_t *obj;

	if (sprite_search_so(path, sizeof(path),name)) {
		fprintf(stderr," can not find %s in search path\n", name);
		return NULL;
	}
	handle = dlopen(path,RTLD_LAZY);
	if (handle == NULL) {
		fprintf(stderr, "can not load object %s\n", path);
		return NULL;
	}
	s = name + strlen(name)-1;
	while((s != name) && *s != '/') s--;
	snprintf(path,sizeof(path), "%s_new", s);
	new = dlsym(handle,path);
	if (new == NULL) {
		fprintf(stderr," Can not find symbol %s at module\n", path);
		return NULL;
	}
	obj = new(rdman, root);
	return obj;
}

/* vim: set ts=4 */
