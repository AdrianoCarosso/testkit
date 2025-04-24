// Parse command line, set up command arguments Unix-style, and call function.
// Note: argument is modified (delimiters are overwritten with nulls)
//

#include <gtk/gtk.h>

#include <string.h>
#include <ctype.h>

#include "cmdpslave.h"

//#define QUOTE   34      // char: "
#define QUOTE   27      // char: '
#define DEC		'|' 	// delimiter char

// Use '|' as argv delimiter
int cmdpslave(struct CMDSLAVE cmds[], char *line){
//register int i ;
struct CMDSLAVE *cmdp;
char *argv[NARG],*cp ;
int argc,qflag;

// 	// Remove cr/lf
// 	if((cp = strchr(line,'\015')) != NULLCHAR)
// 		*cp = '\0';
// 	if((cp = strchr(line,'\012')) != NULLCHAR)
// 		*cp = '\0';       // shouldn't be necessary

	for(argc = 0;argc < NARG;argc++)
		argv[argc] = NULLCHAR;

	for(argc = 0;argc < NARG;){
		qflag = 0;
		//// Skip leading white space
		//while(*line == ' ' || *line == '\t')	line++;
		
		argv[argc] = line ;
		printf("argv[%d]=%s\n", argc, line ) ;

		if(*line == '\0')	break;
		
		// Check for quoted token
		if(*line == QUOTE){
			line++;         // Suppress quote
			qflag = 1;
		}
		argv[argc++] = line;    // Beginning of token
		
		// Find terminating delimiter
		if(qflag){
			// Find quote, it must be present
			if((cp = strchr(line,QUOTE)) == NULLCHAR){
				return -100;
			}
		} else {
			// Find space or tab. If not present,
			// then we've already found the last
			// token.
			//
			if((cp = strchr(line,DEC)) == NULLCHAR)
				//if ((cp = strchr(line,'\t')) == NULLCHAR)
					break;
		}
		*cp++ = '\0';
		line = cp;
	}

	// Ignore line starting with comment qualifier
	//if(argv[0][0] == COMMENT_CHAR)
	//	return 0;

//	for(i=0 ; argv[0][i] ; i++)
//		argv[0][i] = tolower(argv[0][i]) ;

	// Look up command in table; prefix matches are OK
	for(cmdp = cmds;cmdp->name != NULLCHAR;cmdp++){
	//for(cmdp = cmds;cmdp->name ;cmdp++){
		if (strcmp(argv[0],cmdp->name) == 0)
			break;
	}

	if(cmdp->name == NULLCHAR)
		return -100;
	else
		return (*cmdp->func)(argc,argv);
}

// Call a subcommand based on the first token in an already-parsed line
int subcmd(struct CMDSLAVE tab[], int argc, char *argv[])
{
		register int i ;
		register struct CMDSLAVE *cmdp;

		// Strip off first token and pass rest of line to subcommand
		argc--;
		argv++;

		for(i=0 ; argv[0][i] ; i++)
				argv[0][i] = tolower(argv[0][i]) ;

		for(cmdp = tab;cmdp->name != NULLCHAR;cmdp++){
				if(strncmp(argv[0],cmdp->name,strlen(argv[0])) == 0){
						return (*cmdp->func)(argc,argv);
				}
		}
		return -1;
}

