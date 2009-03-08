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
#include "mbapp.h"
#include <animated_menu.h>



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
}MyAppData;

MBApp *myApp;



void myselect(mb_animated_menu_t *m, int select)
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    char path[1024];
    int len,i;

    if (strcmp(data->titles[select],"..")==0) {
	    strcpy(path, data->curDir);
	    len = strlen(path);
	    for(i=len-1;i>0;i--) {
		    if (path[i] == '/') {
			    path[i] = 0;
			    break;
		    }
	    }
    } else {
    	snprintf(path,1024,"%s/%s", data->curDir,data->titles[select]);
    }

    MyApp_fillDirInfo(myApp, path);
}


void mypreview(MyAppData *data, char *path)
{
    redraw_man_t *rdman = MBAPP_RDMAN(myApp);
    mb_img_ldr_t *ldr = rdman_img_ldr(rdman);
    mb_img_data_t *img = MB_IMG_LDR_LOAD(ldr, path);
    shape_t *obj = (shape_t *) MB_SPRITE_GET_OBJ(myApp->rootsprite, "previewimg");

    printf("Preview %s\n",path);
    if (img) {
	    printf("image %d %d\n",img->w,img->h);
	    sh_image_set_img_data(obj,img,0,0,img->w,img->h);
	    rdman_shape_changed(MBAPP_RDMAN(myApp),obj);
            rdman_redraw_changed(MBAPP_RDMAN(myApp));
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
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    char *s = data->titles[select];
    char path[1024];

    printf("check %s\n",s);

    if (endWith(s,".png")) {
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


MyApp_fillDirInfo(MBApp *app,char *curdir)
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    DIR *dir;
    struct dirent *e;
    struct fileinfo *f;
    int i;

    dir = opendir(curdir);
    if (dir == NULL) {
	    printf("We can not open the direftory %s\n", curdir);
	    return;
    }

    if (data->curDir)
	    free(data->curDir);
    data->curDir = strdup(curdir);
    
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
	    if (e->d_type == DT_REG) {
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
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    subject_t *key = MBAPP_keySubject(myApp);
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=myApp->rootsprite;
    
    data->m = mb_animated_menu_new(myApp,myApp->rootsprite,"item",NULL);
    mb_animated_menu_set_callback(data->m, myselect);
    mb_animated_menu_set_update_callback(data->m, myupdate);
    data->curDir = NULL;
    data->nFiles=0;
    MyApp_fillDirInfo(myApp,dir);
    mb_animated_menu_set_speed(data->m,300);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    MyAppData data;
    mb_timeval_t tmo,interval;
    char *dir;

    if (argc > 1)
	    dir = argv[1];
    else
	    dir ="/tmp";
    myApp = MBApp_Init("browser");
    MBApp_setData(myApp,&data);
    MyApp_InitContent(dir);

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
