#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#define MAXSIZE 512
#define NRMSIZE   128

char ParamSel[MAXSIZE] ;		// PM scelti
char Bmom[MAXSIZE];

int GetIntStr(char *Delim, char *sInput,  unsigned int order, char *sOut){
	int i;
	int OverLen=0;
	char cpIn[512], *pStringa;

    memset(cpIn, 0, 512) ;
	i = strlen(sInput) ;
	strncpy(cpIn,sInput, ((i>511)? 511:i));
    char *inputstring=cpIn;
    
	i = 0 ;
    while( (i<order) && (0 != (pStringa = strsep(&inputstring, Delim))) ) {
		printf("AA: %d <%s>\n", i, pStringa) ;
        sprintf(sOut,"%s",pStringa);
        if (0 == *pStringa) {
            sOut[0] = '\0';
        }
        i++;
    }
    if (i> order) OverLen=i;
    
	return OverLen;
}

char * loc_fgets(char * a1, int nn, FILE * abc)
{
char *aa ;
int ii, i ;
	
	aa = fgets(a1, nn, abc); 	
	
	if (aa==NULL) a1[0]='\0';
		
	ii = strlen(a1) ;
// added FR 16/05/23
	for(i=0;i<ii;i++){
		if ((a1[i]<32) || (a1[i]>126)) {
			a1[i] = '\0' ;
			break ;
		}
	}
	
	if (a1[ii-1]==0xa){
		a1[ii-1]='\0' ;
	}
	
	return(aa) ;
}

void togliCR(char *msg)
{
	int i;
	
	for(i=0 ; msg[i] ; i++){ 
		if (msg[i] < ' ') {							// se è un carattere di controllo ...
			msg[i] = '\0';							// ... lo si sostituisce con terminatore di stringa
			break;
		}
	}
}

void RLTrimmwithplace(char *testo)
{
	char *pnome, stemp[MAXSIZE];
	unsigned char c;
	int lungh;

//				StampaDB("RLTrimm---> Stringa da elaborare", testo); // Su finestra DOS di Debug 
	
	if (strlen(testo) > 0) {
		strcpy(stemp, testo);
		pnome = stemp;
		c = pnome[0];
		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') ) || (c==182) ) {
//				StampaDB ("pnome",pnome);					// Su finestra DOS di Debug 
			pnome++;							// toglie i caratteri NON Alfanumerici prima
			c = pnome[0];
		}
		lungh = strlen(pnome);
		c = pnome[lungh-1];
		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') ) || (c==182) ) { // ot tab char
//				StampaDB ("pnome",pnome);		// Su finestra DOS di Debug 
			//pnome[lungh-1]='\0';
			lungh--;							// toglie i caratteri NON Alfanumerici dopo
			c=pnome[lungh-1];
		}
		pnome[lungh+1]='\0';
		strcpy(testo, pnome);
	}
//				StampaDB("RLTrimm---> Stringa elaborata1", pnome); // Su finestra DOS di Debug 
//				StampaDB("RLTrimm---> Stringa elaborata", testo);  // Su finestra DOS di Debug 
}

//int SK_ParamSet(void)
int main(int argc, char *argv[])
{
	//M_SetPar(255,"N.D.");  //Cancello Tutti i Parametri
	FILE *fpar ;
	char M_Par[MAXSIZE] ;
	char Valore[NRMSIZE] ;
	char Parametro[NRMSIZE] ;
	
	strcpy(ParamSel,"PARAMETRI_2044_ACCISE02.par") ;

	
	fpar = fopen(ParamSel, "r");
	if (fpar == NULL) {			// Se il file di Parametri non esiste ...
		fclose(fpar);
		printf("Errore durante la lettura del file: %s\n",ParamSel);
	  	//printf(Bmom);
//	  	call_exit(YES, Bmom);
	}else{													// si leggono parametri
		while (!feof(fpar)) { //fino alla fine del file
			loc_fgets(M_Par, 256, fpar);
			if (strlen(M_Par)==0) break ;
			togliCR(M_Par);
			GetIntStr("=", M_Par, 1, Parametro);
			GetIntStr("=", M_Par, 2, Valore); 
			printf("Pre-Par1: <%s> -> <%s><%s><%ld>\n",M_Par, Parametro, Valore, strlen(Valore));
			//RLTrimmwithplace(Valore);						// toglie i caratteri non alfanumerici prima e dopo !
			//printf("Pre-Par2: <%s> -> <%s><%s><%ld>\n",M_Par, Parametro, Valore, strlen(Valore) );
			//printf(Bmom);
			//M_SetPar(atoi(Parametro), Valore);
		}
		fclose(fpar);
	}                                              
	return 0;
}
