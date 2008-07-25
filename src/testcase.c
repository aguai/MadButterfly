#include <CUnit/Basic.h>

extern CU_pSuite get_coord_suite(void);
extern CU_pSuite get_geo_suite(void);
extern CU_pSuite get_shape_path_suite(void);

int
main(int argc, char * const argv[]) {
    CU_pSuite suite;

    if(CU_initialize_registry() != CUE_SUCCESS)
	return CU_get_error();

    get_coord_suite();
    get_geo_suite();
    get_shape_path_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
