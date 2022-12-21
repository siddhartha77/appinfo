#include "constants.h"

#include "utils.h"

void ErrorAlert(short stringListID, Error error)
{	
	Str255	            param, errorStr, lineStr, errorFileStr;
	Str255              debugStr = "\p";
    StringHandle        strH;
    long                errorFileLen;
    
    if (DEBUG_ERROR_VERBOSE)
    {
        errorFileLen = cStrLen(error.file, 0xff);
        BlockMove(error.file, errorFileStr + 1, errorFileLen);
        errorFileStr[0] = errorFileLen;
        
        NumToString (error.line, lineStr);
        myCopyPStr(errorFileStr, debugStr);
        myAppendPStr(debugStr, "\p:");
        myAppendPStr(debugStr, lineStr);
    }

	if (error.err > 0)
	{
	    GetIndString (param, stringListID, error.err);
	    NumToString (error.err + stringListID, errorStr);
	    ParamText (param, errorStr, debugStr, NULL);
	}
	else
	{
	    strH = GetString(error.err);
	    if (strH == NULL) strH = GetString(kUndefinedErrorID);
	    NumToString (error.err, errorStr);
	    ParamText (*strH,  errorStr, debugStr, NULL);
	}
		
	CenterAlert (kAlertError);
	(void) StopAlert (kAlertError, NULL);
}

void mySafeFilename(Str255 s)
{
    #define         MAX_FILENAME_LEN    31
    #define         MAX_EXTENSION_LEN   8
    
    register int    i;
    
    int             extensionRIndex = 0;
    
    // Truncate filename and add ellipsis if necessary
    if (s[0] > MAX_FILENAME_LEN)
    {
        for (i = s[0] ; i > s[0] - MAX_EXTENSION_LEN ; --i)
        {
            if (s[i] == '.')
            {
                extensionRIndex = s[0] - i + 1;
                break;
            }
        }
        
        while (s[0] > MAX_FILENAME_LEN) {
            myDeleteElementFromPStr(s, s[0] - extensionRIndex);
        }
        
        s[s[0] - extensionRIndex] = '.';
    }
}

Error myCreateError(OSErr err, char *file, long line)
{
    Error   error;
    
    error.err = err;
    error.line = line;
    error.file = (unsigned char *)file;
    
    return error;
}

void myAlertNumber(long n) {
    Str255  buff;
    
    NumToString(n, buff);
    ParamText(buff, NULL, NULL, NULL);
    NoteAlert(kAlertError, NULL);
}

void myAlertPStr(StringPtr s) {
    ParamText(s, NULL, NULL, NULL);
    NoteAlert(kAlertError, NULL);
}

void myAlertFourCharCode(FourCharCode c) {
    char    buff[5];
    
    buff[0] = 4;
    *(FourCharCode *)(buff + 1) = c;
    
    ParamText((StringPtr)buff, NULL, NULL, NULL);
    NoteAlert(kAlertError, NULL);
}

unsigned char myValToBaseXChar(unsigned short v)
{
	if (v < 10) return '0' + v;		    // 00..09  -> '0'..'9'
	if (v < 36) return 'A' - 10 + v;	// 10..35  -> 'A'..'Z'
	return 'a' - 36 + v;				// 36..61+ -> 'a'..'z'+
}

void myCopyPStr(const Str255 s, Str255 t)
{
	BlockMove((Ptr)s, (Ptr)t, s[0] + 1);
}

void myPrefixPStr(Str255 s, const Str255 prefixStr)
{
    register unsigned short	i = s[0];
    register unsigned short	j = prefixStr[0];
	
	if (j)
	{
		if ((i+j) <= 255)       // if room to prefix
		{
			BlockMove((Ptr)&s[1], (Ptr)&s[j + 1], i);
			BlockMove((Ptr)&prefixStr[1], (Ptr)&s[1], j);	
			s[0] += j;
		}
		else DebugStr("\pOverflow");
	}
}


void myAppendPStr(Str255 s, const Str255 suffixStr)
{
    register unsigned short	i = s[0];
    register unsigned short	j = suffixStr[0];
	
	if (j)
	{
		if ((i + j) <= 255)
		{
			BlockMove((Ptr)&suffixStr[1], (Ptr)&s[i + 1], j);
			s[0] += j;
		}
		else DebugStr("\pOverflow");
	}
}

void myAppendCharToPStr(Str255 s, unsigned char c)
{
	if (s[0] < 255) s[++s[0]] = c;
	else DebugStr("\pOverflow");
}

void myUNumToBaseBPStr(unsigned long n, Str255 s, unsigned short b, unsigned short minDigits)
{ 
	s[0] = myUNumToBaseBDigits(n, &s[1], b, minDigits);
}

unsigned short myUNumToBaseBDigits(unsigned long n, StringPtr s, unsigned short b, unsigned short minDigits)
{
    register char	numStr[32];		// 32 binary digits are possible from unsigned 32 bits
    register short	start, i, j = 0;

	if ((b<2) || (b>62)) return 0;	// error - illegal base was passed
	do
	{
		numStr[j++] = myValToBaseXChar(n % b);
		n /= b;
	} while (n > 0);
	start = (j < minDigits) ? (minDigits - j) : 0;
	for (i = 0 ; i < start ;i++) s[i] ='0';
	for (i = start ; j > 0 ;i++) s[i] = numStr[--j];
	return i;
}

void myDeleteElementFromPStr(Str255 s, unsigned short index)
{
    register unsigned short		i, len = s[0];

	if (index <= len)
	{
		for (i = index ; i < len ; i++) s[i] = s[i + 1];
		s[0]--;
	}
}

unsigned long cStrLen(unsigned char *s,unsigned long maxLen)
{
    register unsigned char *c=s;
    register unsigned long len=0;
	
	while ((len < maxLen) && ((Byte *) *(c++)!=0x00)) len++;
	return len;
}

void myCStrToPStr(unsigned char *s)
{
    unsigned char	*t = s;
    unsigned char	*p = s;
    unsigned long	len;

	while (*t != 0) t++;
	len = t - s;
	p = t;
	while (p != s) *(p--) = *(--t);
	if (len > 255)
	{
		DebugStr("\pmyCStrToPStr: string too long");
		len = 255;
	}
	s[0] = len;
}

Boolean myMemCompare(unsigned char *a, unsigned char *b, short len) {
    register short  i;
    
    for (i = 0 ; i < len ; ++i) {
        if (*(a + i) != *(b + i)) return false;
    }
    
    return true;
}