// ****************************************************************************


#define SET_PIN_INPUT   0x2
#define SET_PIN_OUT_0   0x0
#define SET_PIN_OUT_1   0x1

#define PIN_POSITION_PORT   29
#define PIN_MASK_PORT       (0x7<<PIN_POSITION_PORT)

#define PIN_POSITION_NUM    24
#define PIN_MASK_NUM        (0x1f<<PIN_POSITION_NUM)

#define PIN_POSITION_SET    22
#define PIN_MASK_SET        (0x3<<PIN_POSITION_SET)

#define SET_PORT_VAL(PORT,PNUM,PSET,PMASK)              \
  ( (((PORT)<<PIN_POSITION_PORT) & PIN_MASK_PORT) |     \
    (((PNUM)<<PIN_POSITION_NUM)  & PIN_MASK_NUM)  |     \
    (((PSET)<<PIN_POSITION_SET)  & PIN_MASK_SET)  |     \
    (PMASK) )

#define SET_PORT_PU     0x30    // Hysteresis and Pull up
#define SET_PORT_PD     0x28    // Hysteresis and Pull down
#define SET_PORT_PN     0x20    // Hysteresis and Pull none
#define SET_PORT_OD     0x420   // Hysteresis and Pull none and OpenDrain
#define SET_PORT_AD     0x00    // Pull none and ADC
#define SET_PORT_DA     0x10000 // Pull none and DAC
#define SET_NO_FILTER   0x300   // No filter , no SLEW
#define SET_PIOW_NORMAL 0x80    // Normal operation for PIO type W
// PIO 0.7,0.8,0.9 will be set bit 7 of PCON for normal operation

#ifdef POPULATE_PORTARRAY

#ifdef M3025
#include "portat_M3025.h"
#endif // M3025

#ifdef GEMHH
#include "portat_GEMHH.h"
#endif // GEM_HH

#endif // POPULATE_PORTARRAY
