#include <mb.h>
#include <mbapp.h>
MBApp *MBApp_Init(const char *module, const char *module_dir)
{
    MBApp *app = (MBApp *) malloc(sizeof(MBApp));
    X_MB_runtime_t *rt;

    rt = X_MB_new(":0.0", 800, 600);
    if(rt == NULL)
	return NULL;

    sprite_set_search_path(module_dir);

    app->rt = rt;
    app->rdman =  X_MB_rdman(rt);
    app->kbevents = X_MB_kbevents(rt);
    
    app->rootsprite= sprite_load(module,app->rdman, app->rdman->root_coord);
    if(app->rootsprite == NULL) {
	X_MB_free(rt);
	free(app);
	return NULL;
    }
    
    rdman_attach_backend(app->rdman, rt);
    MB_SPRITE_GOTO_SCENE(app->rootsprite, 1);
    
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
