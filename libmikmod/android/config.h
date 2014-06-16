/* Customized config.h for Android builds */

/* Define if the OpenSL ES driver is compiled */
#define DRV_OSLES 1

/* Define if your system provides POSIX.4 threads */
#define HAVE_PTHREAD 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#undef WORDS_BIGENDIAN

#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1

/* Version number of package */
#define VERSION "3.3.7"
