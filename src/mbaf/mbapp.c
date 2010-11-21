// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <mb.h>
#include <mb_af.h>
#include <mb_backend.h>

mbaf_t *mbaf_init(const char *module, const char *module_dir)
{
    mbaf_t *app = (mbaf_t *) malloc(sizeof(mbaf_t));
    mb_rt_t *rt;

    rt = backend.new(":0.0", 800, 600);
    if(rt == NULL)
	return NULL;

    sprite_set_search_path(module_dir);

    app->rt = rt;
    app->rdman =  backend.rdman(rt);
    app->kbevents = backend.kbevents(rt);

    app->rootsprite= sprite_load(module,app->rdman, app->rdman->root_coord);
    if(app->rootsprite == NULL) {
	backend.free(rt);
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

mb_timer_man_t *mbaf_get_timer(mbaf_t *app)
{
    return backend.timer_man(app->rt);
}

void mbaf_loop(mbaf_t *app)
{
    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    backend.loop(app->rt);

    /*
     * Clean
     */
    backend.free(app->rt);
    free(app);
}
