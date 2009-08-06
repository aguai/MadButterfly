#include <stdio.h>
#include <mb.h>
#include "mb_af.h"
#include "mb_ani_menu.h"
#include "%n.h"

void myselect_callback(mb_animated_menu_t *m, int select)
{
    printf("menu %d is selected\n", select);
    // Put the callback function for menu select here
}

void MyApp_InitContent(mbaf_t *app,int argc,char *argv[])
{
   // This function is called when the application is started.
}
