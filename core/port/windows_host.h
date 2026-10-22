/* Force-included only in the Windows host build of boot_test: QUASI88's unix
 * file layer calls the POSIX two-argument mkdir(), which mingw spells _mkdir(). */
#ifndef KAIRO88_WINDOWS_HOST_H
#define KAIRO88_WINDOWS_HOST_H

#include <direct.h>
#include <io.h>
#define mkdir(path, mode) _mkdir(path)

#endif
