
/* The following types must have exact sizes: */

typedef unsigned char byte;        /* MUST be 8 bits */
typedef unsigned short word16;     /* MUST be 16 bits */

void LLCRC_Init(byte *table);        /* Initialise. Passed the address of */
                                   /* a 512-byte buffer used for workspace */
void LLCRC_Clear(void);              /* Reset the CRC */ 
void LLCRC_ClearTo(word16 value);	   /* Reset to a non-standard value */ 
void LLCRC_Update(byte a);           /* Add a byte to the CRC */
word16 LLCRC_Done(void);             /* Get the completed CRC */
byte *LLCRC_Table(void);             /* Return the workspace address */

