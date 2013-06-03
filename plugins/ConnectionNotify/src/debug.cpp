#include "debug.h"

#define MAX_LENGTH 512


void _OutputDebugString(TCHAR* lpOutputString, ... )
{
    TCHAR OutMsg[MAX_LENGTH];
    TCHAR format[MAX_LENGTH];
	int i,j;
    va_list argptr;    
    
    va_start( argptr, lpOutputString );            
    

    
    for(i=0,j=0;lpOutputString[i] != '\0';i++) 
    {
        format[j++] = lpOutputString[i];
		format[j] = '\0';
        // If escape character
        //if(lpOutputString[i] == '\\')
        //{
        //    i++;
        //    continue;
        //}
        // if not a substitutal character
        if(lpOutputString[i] != '%')
            continue;
        
        format[j++] = lpOutputString[++i];
        format[j] = '\0';
        switch(lpOutputString[i])
        {
            // string
        case 's':
            {
                TCHAR* s = va_arg( argptr, TCHAR * );
                _stprintf(OutMsg,format,s);
                _tcsncpy(format,OutMsg,_countof(OutMsg));
                j = _tcslen(format);
                _tcscat(format,_T(" "));
                break;
            }
            // character
        case 'c':
            {
                char c = (char) va_arg( argptr, int );
                _stprintf(OutMsg,format,c);
                _tcsncpy(format,OutMsg,_countof(OutMsg));
                j = _tcslen(format);
                _tcscat(format,_T(" "));
                break;
            }
            // integer
        case 'd':
            {
                int d = va_arg( argptr, int );
                _stprintf(OutMsg,format,d);
                _tcsncpy(format,OutMsg,_countof(OutMsg));
                j = _tcslen(format);
                _tcscat(format,_T(" "));
                break;
            }
        }
		format[j+1] = '\0';
    }
	_tcscat(format,_T("\n"));
    OutputDebugString(format);

    va_end( argptr );

}


