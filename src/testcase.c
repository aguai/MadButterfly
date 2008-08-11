#include <CUnit/Basic.h>

extern CU_pSuite get_tools_suite(void);
extern CU_pSuite get_coord_suite(void);
extern CU_pSuite get_geo_suite(void);
extern CU_pSuite get_shape_path_suite(void);
extern CU_pSuite get_redraw_man_suite(void);
extern CU_pSuite get_animate_suite(void);

int
main(int argc, char * const argv[]) {
    CU_pSuite suite;

    if(CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();

    suite = get_tools_suite();
    if(suite == NULL)
	return CU_get_error();
    suite = get_coord_suite();
    if(suite == NULL)
	return CU_get_error();
    suite = get_geo_suite();
    if(suite == NULL)
	return CU_get_error();
    suite = get_shape_path_suite();
    if(suite == NULL)
	return CU_get_error();
    suite = get_redraw_man_suite();
    if(suite == NULL)
	return CU_get_error();
    suite = get_animate_suite();
    if(suite == NULL)
	return CU_get_error();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
