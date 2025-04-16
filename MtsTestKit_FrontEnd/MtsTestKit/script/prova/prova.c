#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

unsigned char atoh (unsigned char data)
{ if (data > '9') 
    { data += 9;
    }
    return (data &= 0x0F);
}

int64_t stringascii_to_hex(char *is_ascii_string)
{
   int len,i,num;
	 int64_t hex;
	 hex=0;
   len=strlen(is_ascii_string);
	 if (len>8) len=8;
	 //len=1;
   for(i = 0; i<len; i++){
		num=atoh(is_ascii_string[i]);
		hex=(hex << 4) + num;
   }
	 return hex;
}

int main()
{
	 int64_t checksum;
	 char asciihexstring[10];
	 char hexstring[20];
	 strcpy(asciihexstring,"8dab");
	 //strcpy(asciihexstring,"8000");
   // printf() displays the string inside quotation
   printf("Hello, World!\n");
	 printf("CheckSum:0x%s\n",asciihexstring);
	 checksum=stringascii_to_hex(asciihexstring);
	 printf("CheckSum:0x%x\n",checksum);
	 if (checksum !=0) printf("CheckSum:0x%x\n",checksum);
	 printf("Exit!\n");
   return 0;
}
