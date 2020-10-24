#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#ifdef __GNUG__
#pragma interface
#endif

#include <stdbool.h>
#include "m_fixed.h"

extern int ms_to_next_tick;
int I_StartDisplay(void);
void I_EndDisplay(void);
int I_GetTime_RealTime(void);     /* killough */
#ifndef PRBOOM_SERVER
fixed_t I_GetTimeFrac (void);
#endif
void I_GetTime_SaveMS(void);

unsigned long I_GetRandomTimeSeed(void); /* cphipps */

void I_uSleep(unsigned long usecs);

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer
 */
const char* I_GetVersionString(char* buf, size_t sz);

/* cphipps - I_SigString
 * Returns a string describing a signal number
 */
const char* I_SigString(char* buf, size_t sz, int signum);

const char *I_DoomExeDir(void); // killough 2/16/98: path to executable's dir

char* I_FindFile(const char* wfname, const char* ext);

/* cph 2001/11/18 - wrapper for read(2) which deals with partial reads */
void I_Read(int fd, void* buf, size_t sz);

/* cph 2001/11/18 - Move W_Filelength to i_system.c */
int I_Filelength(int handle);

void I_SetAffinityMask(void);


int doom_main(int argc, char const * const * argv);

int I_Lseek(int fd, off_t offset, int whence);
int I_Open(const char *wad, int flags);
void I_Close(int fd);


//HACK mmap support, w_mmap.c
#define PROT_READ 1
#define MAP_SHARED 2
#define MAP_FAILED (void*)-1

void *I_Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int I_Munmap(void *addr, size_t length);

int isValidPtr(void *ptr);

bool I_InitSD();
void I_InitStorage();

#endif
