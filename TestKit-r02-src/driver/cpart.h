/* cpart.h - RTXC Partition include file */

#define NAMMAX 8

#define NPARTS 1

#define PKTBUF   1 /* Packet buffer        */
extern const MAP nparts ;

extern const PKHEADER pkkheader[1+NPARTS] ;

extern PHEADER pheader[1+NPARTS] ;

#ifdef CBUG
extern const char partkname[1+NPARTS][NAMMAX+1] ;

#endif // CBUG
