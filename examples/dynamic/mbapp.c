#include <mb.h>
#include <mbapp.h>
MBApp *MBApp_Init(char *module)
{
    MBApp *app = (MBApp *) malloc(sizeof(MBApp));
    X_MB_runtime_t *rt;

    rt = X_MB_new(":0.0", 800, 600);

    app->rt = rt;
    app->rdman =  X_MB_rdman(rt);
    app->rootsprite= sprite_load(module,app->rdman, app->rdman->root_coord);
    return app;
}

void MBApp_setData(MBApp *app,void *data)
{
    app->private = (void *) data;
}

mb_tman_t *MBApp_getTimer(MBApp *app)
{
    return X_MB_tman(app->rt);
}

void MBApp_loop(MBApp *en)
{
    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(en->rt);

    /*
     * Clean
     */
    X_MB_free(en->rt);
    free(en);
}
