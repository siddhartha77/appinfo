#ifndef __UTILS_H__
#define __UTILS_H__

#define ERROR(err) \
    myCreateError(err, __FILE__, __LINE__)

struct Error {
    OSErr           err;
    long            line;
    unsigned char   *file;
};
typedef struct Error Error;

void            CenterAlert (short theID);
void            ErrorAlert(short stringListID, Error error);

void            mySafeFilename(Str255 s);
Error           myCreateError(OSErr err, char *file, long line);
void            myAlertNumber(long n);
void            myAlertPStr(StringPtr s);
void            myAlertFourCharCode(FourCharCode c);

unsigned char   myValToBaseXChar(unsigned short v);
void            myCopyPStr(const Str255 s,Str255 t);
void            myPrefixPStr(Str255 s,const Str255 prefixStr);
void            myAppendPStr(Str255 s,const Str255 suffixStr);
void            myAppendCharToPStr(Str255 s,unsigned char c);
void            myUNumToBaseBPStr(unsigned long n,Str255 s,unsigned short b,unsigned short minDigits);
unsigned short  myUNumToBaseBDigits(unsigned long n,StringPtr s,unsigned short b,unsigned short minDigits);
void            myDeleteElementFromPStr(Str255 s,unsigned short index);
unsigned long   cStrLen(unsigned char *s,unsigned long maxLen);
void            myCStrToPStr(unsigned char *s);
Boolean         myMemCompare(unsigned char *a, unsigned char *b, short len);

#endif
