#ifndef ___TIMING_H___
#define ___TIMING_H___

#include <stdbool.h>

#define DALI_BAUD           (1200)
#define DALI_BIT_USECS      (1000000.0/DALI_BAUD)
#define DALI_HALF_BIT_USECS (DALI_BIT_USECS/2.0)
#define DALI_MARGIN_USECS   (45)
#define USEC_TO_TICKS(u)    ((uint16_t) (((float)u)*(F_CPU/1000000.0) + 0.5))
#define MSEC_TO_TICKS(u)    USEC_TO_TICKS((u)*1000)
#define TICKS_TO_USECS(u)   (uint16_t) ((u)/(F_CPU/1000000.0))

// Reponse delay is 22 half bits, or 9.17 msec
#define DALI_RESPONSE_MAX_DELAY_USEC (22 * DALI_HALF_BIT_USECS)

// We allow 45 uSec variation in timing (371 - 461)
static inline bool isHalfBit(uint16_t v) {
    return v >= USEC_TO_TICKS(DALI_HALF_BIT_USECS-DALI_MARGIN_USECS) && v <= USEC_TO_TICKS(DALI_HALF_BIT_USECS+DALI_MARGIN_USECS);
}

// We allow 45 uSec variation in timing (788 - 878)
static inline bool isFullBit(uint16_t v) {
    return v >= USEC_TO_TICKS(DALI_BIT_USECS-DALI_MARGIN_USECS) && v <= USEC_TO_TICKS(DALI_BIT_USECS+DALI_MARGIN_USECS);
}



#endif