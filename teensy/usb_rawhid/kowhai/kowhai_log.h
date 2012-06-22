#ifndef _KOWHAI_LOG_H_
#define _KOWHAI_LOG_H_


#define KOW_ERR "ERROR: "

#ifdef KOWHAI_DBG

void kowhai_log(char* msg, ...);

#ifdef __GNUC__
#define KOW_LOG(msg, s...) kowhai_log(msg, ## s)
#else
#define KOW_LOG(msg, ...) kowhai_log(msg, __VA_ARGS__)
#endif

#else

#define KOW_LOG(msg, ...)

#endif

#endif

