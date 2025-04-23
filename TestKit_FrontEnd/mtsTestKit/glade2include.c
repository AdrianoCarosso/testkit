//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//


#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

int main(int  argc, char *argv[] )
{
int i, j ;
FILE *f_in, *f_out ;
char buf_in[512], buf_out[1024] ;

	if (argc<3) printf("Usage:  %s <input_file> <output_file>.h (string name=output_file)\n", argv[0]) ;

	f_in = fopen(argv[1], "r") ;
	if (!f_in){
		printf("Can't open %s\n", argv[1]) ;
		return(1) ;
	}

	sprintf(buf_in, "%s.h", argv[2]) ;
	f_out = fopen(buf_in, "w") ;
	if (!f_out){
		printf("Can't create %s.h\n", argv[2]) ;
		fclose(f_in) ;
		return(1) ;
	}

	fclose(f_out) ;
	f_out = fopen(buf_in, "a") ;         

	fprintf(f_out,"static const char %s[] =", argv[2]) ;
	
	while(fgets(buf_in, 500, f_in)) {        // get lines
		j = 0 ;
		for(i=0;i<(strlen(buf_in)-1);i++){
			if(buf_in[i]=='"'){
				buf_out[j++]='\\' ;
			}
			buf_out[j++] = buf_in[i] ;
		}
		buf_out[j]='\0' ;
		fprintf(f_out, "\n\"%s\"", buf_out) ;
	}
	fprintf(f_out, ";\n") ;
	
	fclose(f_in) ;
	fclose(f_out) ;
	
    return( 0 );
}



