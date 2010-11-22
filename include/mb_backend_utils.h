#ifndef __MB_BACKEND_UTILS_H_
#define __MB_BACKEND_UTILS_H_

#include "mb_backend.h"
#include "mb_timer.h"

/*! \brief A facotry of timer manager implemented with mb_tman_t.
 */
extern mb_timer_factory_t tman_timer_factory;
extern mb_tman_t * tman_timer_man_get_tman(mb_timer_man_t *tm_man);

#endif /* __MB_BACKEND_UTILS_H_ */
