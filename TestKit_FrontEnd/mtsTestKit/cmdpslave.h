#define	NARG			30		// Max number of args to commands

struct CMDSLAVE	{
		char *name;
		int  (*func)();
};

#ifndef	NULLCHAR
#define	NULLCHAR		(char *)0
#endif

#define	COMMENT_CHAR	'#'

extern int cmdpslave(struct CMDSLAVE cmds[], char *line);
extern int subcmd(struct CMDSLAVE tab[], int argc, char *argv[]) ;

