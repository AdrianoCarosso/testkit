/* cpart.c - RTXC Partition definitions - ARM */

#include "typedef.h"
#include "rtxstruc.h"

#include "cpart.h"

#define M1SZ 32         // PKTBUF   - Packet buffer size

#if defined(USE_AT91SAM7A3)
#define M1CT 192         // PKTBUF   - Packet buffer number (old 64)
#elif defined(USE_LPC17XX) 
#define M1CT 192         // PKTBUF   - Packet buffer number (old 64)
#elif defined(USE_LPC1788)
#define M1CT 256         // PKTBUF   - Packet buffer number (old 64)
#elif defined(USE_AT91SAM3S4)
#define M1CT 192         // PKTBUF   - Packet buffer number (old 64)
#else
#define M1CT 128         // PKTBUF   - Packet buffer number (old 64)
#endif
const MAP nparts = NPARTS;

static char part1[M1CT*M1SZ];

PHEADER pheader[1+NPARTS];

const PKHEADER pkkheader[1+NPARTS] =
{
   { (struct xmap *)0, (size_t)0, 0 },
   { (struct xmap *)&part1[0], (size_t)M1SZ, M1CT }
};

#ifdef CBUG
const char partkname[1+NPARTS][NAMMAX+1] =
{
   " ",
   "PKTBUF"
};
#endif // CBUG
