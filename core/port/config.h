#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* QUASI88 system configuration for the Kairo88 port. The port is driven one
 * frame at a time by kairo88_core.c; there is no window, event loop or timer of
 * its own. */

#ifndef QUASI88_KAIRO
#define QUASI88_KAIRO
#endif

#define LSB_FIRST

#define Q_COMMENT "Kairo88 port"

#define SUPPORT_32BPP

#ifndef INLINE
#define INLINE static inline
#endif

#endif /* CONFIG_H_INCLUDED */
