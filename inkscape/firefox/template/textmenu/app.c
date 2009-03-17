#include <stdio.h>
#include <mb.h>
#include "mbapp.h"
#include "animated_menu.h"
#include "app.h"

void myselect_callback(mb_animated_menu_t *m, int select)
{
    printf("menu %d is selected\n", select);
    // Put the callback function for menu select here
}

void MyApp_InitContent(MBApp *app,int argc,char *argv[])
{
   // This function is called when the application is started.
}
