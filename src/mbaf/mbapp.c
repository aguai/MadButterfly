#include <mb.h>
#include <mb_af.h>

mbaf_t *mbaf_init(const char *module, const char *module_dir)
{
    mbaf_t *app = (mbaf_t *) malloc(sizeof(mbaf_t));
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

void mbaf_set_data(mbaf_t *app,void *data)
{
    app->private = (void *) data;
}

mb_tman_t *mbaf_get_timer(mbaf_t *app)
{
    return X_MB_tman(app->rt);
}

void mbaf_loop(mbaf_t *app)
{
    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(app->rt);

    /*
     * Clean
     */
    X_MB_free(app->rt);
    free(app);
}
