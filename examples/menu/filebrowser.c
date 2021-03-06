/*! \file
 *
 *  This is the demo program for the animated menu. We will use to test the MBAF API.
 *  We need to have group item1-item9 in the SVG file. Initially, we will show
 *  item1-item8 only. When a up/down key is pressed, we will draw the next item in item9 and 
 *  add two words to move item1-item9 smoothly. The first word move items to the 3/4 position 
 *  fastly. The second will move it from 3/4 to the final position slowly to make retard effect.
 *
 *  If we press another key before the second words finish, we will delete the word and replace 
 *  it with a word to move it fastly to the final position and then we will repeat the procedure
 *  to add another two words to move it to the next position.
 */
#include <stdio.h>
#include <mb.h>
#include <string.h>
#include <dirent.h>
//#include "menu.h"
#include "mb_af.h"
#include <mb_ani_menu.h>



struct fileinfo {
	int height;
	int width;
	int depth;
	int bitrate;
	int duration;
	int year;
	char *comment;
};
#define MAX_ENTRY 1000
typedef struct {
	mb_animated_menu_t *m;
	char *curDir;
	struct fileinfo *files[MAX_ENTRY];
	char *titles[MAX_ENTRY];
	int nFiles;
}app_data_t;

mbaf_t *app;



void myselect(mb_animated_menu_t *m, int select)
{
    app_data_t *data = MBAF_DATA(app,app_data_t);
    char path[1024];
    int len,i;

    if (strcmp(data->titles[select],"..")==0) {
	    strcpy(path, data->curDir);
	    len = strlen(path);
	    for(i=len-2;i>0;i--) {
		    if (path[i] == '/') {
			    path[i] = 0;
			    break;
		    }
	    }
    } else {
    	snprintf(path,1024,"%s%s", data->curDir,data->titles[select]);
    }

    MyApp_fillDirInfo(app, path);
}


void mypreview(app_data_t *data, char *path)
{
    redraw_man_t *rdman = MBAF_RDMAN(app);
    paint_t *paint, *old_paint;
    shape_t *obj = (shape_t *) MB_SPRITE_GET_OBJ(app->rootsprite, "previewimg");
    int w, h;

    printf("Preview %s\n",path);
    paint = rdman_img_ldr_load_paint(rdman, path); /* return a cached
						    * paint if the
						    * path was loaded
						    * before */
    if (paint) {
	paint_image_get_size(paint, &w, &h);
	printf("image %d %d\n",w, h);
	old_paint = sh_get_fill(obj);
	rdman_paint_fill(rdman, paint, obj);
	if(old_paint != paint)
	    rdman_paint_free(rdman, old_paint);
	    
	rdman_shape_changed(MBAF_RDMAN(app),obj);
	rdman_redraw_changed(MBAF_RDMAN(app));
    }
}

int endWith(char *path, char *ext)
{
    int i;
    char *s;

    s = path+strlen(path)-1;
    for(i=strlen(ext)-1;i>=0;i--) {
	    if (*s != ext[i]) return 0;
	    s--;
	    if (s < path) return 0;
    }
    return 1;
}

void myupdate(mb_animated_menu_t *m, int select)
{
    app_data_t *data = MBAF_DATA(app,app_data_t);
    char *s = data->titles[select];
    char path[1024];

    printf("check %s\n",s);

    if (endWith(s,".png") || endWith(s, ".jpg") ||
	endWith(s, ".PNG") || endWith(s, ".JPG")) {
    	    snprintf(path,1024,"%s%s", data->curDir,data->titles[select]);
	    mypreview(data,path);
    }
}



struct fileinfo *fileinfo_new()
{
    struct fileinfo *f = (struct fileinfo *) malloc(sizeof(struct fileinfo));

    f->width = 0;
    f->height = 0;
    f->depth = 0;
    f->bitrate = 0;
    f->duration = 0;
    f->year = 0;
    f->comment = NULL;
    return f;
}

void fileinfo_free(struct fileinfo *f)
{
    free(f);
}


MyApp_fillDirInfo(mbaf_t *app,char *curdir)
{
    app_data_t *data = MBAF_DATA(app,app_data_t);
    DIR *dir;
    struct dirent *e;
    struct fileinfo *f;
    char *path;
    int sz;
    int i;

    dir = opendir(curdir);
    if (dir == NULL) {
	    printf("We can not open the direftory %s\n", curdir);
	    return;
    }

    if (data->curDir)
	    free(data->curDir);
    
    sz = strlen(curdir);
    if(curdir[sz - 1] != '/')
	sz++;
    data->curDir = (char *)malloc(sz + 1);
    strcpy(data->curDir, curdir);
    if(curdir[sz - 1] == '\0')
	strcat(data->curDir, "/");
    
    if (data->files) {
	    for(i=0;i<data->nFiles;i++) {
		    fileinfo_free(data->files[i]);
		    free(data->titles[i]);
	    }    
    }
    data->nFiles = 0;
    while(e = readdir(dir)) {
	    if (strcmp(e->d_name,".")==0) continue;
	    if (e->d_type == DT_DIR) {
		    if (data->nFiles < MAX_ENTRY) {
			    f = fileinfo_new();
			    data->files[data->nFiles] = f;
			    data->titles[data->nFiles++] = strdup(e->d_name);
			    printf("%s\n", e->d_name);
		    }
	    }
    }

    closedir(dir);
    dir = opendir(curdir);
    while(e = readdir(dir)) {
	    if (strcmp(e->d_name,".")==0) continue;
	    printf("e->d_type=%d %d name=%s\n",e->d_type,DT_REG,e->d_name);
	    if (e->d_type == DT_REG || e->d_type == DT_LNK) {
		    if (data->nFiles < MAX_ENTRY) {
			    f = fileinfo_new();
			    data->files[data->nFiles] = f;
			    data->titles[data->nFiles++] = strdup(e->d_name);
			    printf("%s\n", e->d_name);
		    }
	    }
    }
    closedir(dir);
    data->titles[data->nFiles] = NULL;
    mb_animated_menu_set_titles(data->m,data->titles);
}


MyApp_InitContent(char *dir)
{
    app_data_t *data = MBAF_DATA(app,app_data_t);
    subject_t *key = MBAF_KB_SUBJECT(app);
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=app->rootsprite;
    
    data->m = mb_animated_menu_new(app,app->rootsprite,"item",NULL);
    mb_animated_menu_set_callback(data->m, myselect);
    mb_animated_menu_set_update_callback(data->m, myupdate);
    data->curDir = NULL;
    data->nFiles=0;
    MyApp_fillDirInfo(app,dir);
    mb_animated_menu_set_speed(data->m,300);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    app_data_t data;
    mb_timeval_t tmo,interval;
    char *dir;

    if (argc > 1)
	    dir = argv[1];
    else
	    dir ="/tmp";
    app = mbaf_init("browser", ".libs");
    mbaf_set_data(app,&data);
    MyApp_InitContent(dir);

    mbaf_loop(app);

    return 0;
}

/* vim: set ts=4 */
