/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "bitops.h"
#include "convert.h"
#include "shared.h"
#include "inc_hash_constants.h"
#include "cpu_des.h"

#if defined (__CYGWIN__)
#include <sys/cygwin.h>
#endif

static const char *PA_000 = "OK";
static const char *PA_001 = "Ignored due to comment";
static const char *PA_002 = "Ignored due to zero length";
static const char *PA_003 = "Line-length exception";
static const char *PA_004 = "Hash-length exception";
static const char *PA_005 = "Hash-value exception";
static const char *PA_006 = "Salt-length exception";
static const char *PA_007 = "Salt-value exception";
static const char *PA_008 = "Salt-iteration count exception";
static const char *PA_009 = "Separator unmatched";
static const char *PA_010 = "Signature unmatched";
static const char *PA_011 = "Invalid hccapx file size";
static const char *PA_012 = "Invalid hccapx eapol size";
static const char *PA_013 = "Invalid psafe2 filesize";
static const char *PA_014 = "Invalid psafe3 filesize";
static const char *PA_015 = "Invalid truecrypt filesize";
static const char *PA_016 = "Invalid veracrypt filesize";
static const char *PA_017 = "Invalid SIP directive, only MD5 is supported";
static const char *PA_018 = "Hash-file exception";
static const char *PA_019 = "Hash-encoding exception";
static const char *PA_020 = "Salt-encoding exception";
static const char *PA_021 = "Invalid LUKS filesize";
static const char *PA_022 = "Invalid LUKS identifier";
static const char *PA_023 = "Invalid LUKS version";
static const char *PA_024 = "Invalid or unsupported LUKS cipher type";
static const char *PA_025 = "Invalid or unsupported LUKS cipher mode";
static const char *PA_026 = "Invalid or unsupported LUKS hash type";
static const char *PA_027 = "Invalid LUKS key size";
static const char *PA_028 = "Disabled LUKS key detected";
static const char *PA_029 = "Invalid LUKS key AF stripes count";
static const char *PA_030 = "Invalid combination of LUKS hash type and cipher type";
static const char *PA_031 = "Invalid hccapx signature";
static const char *PA_032 = "Invalid hccapx version";
static const char *PA_033 = "Invalid hccapx message pair";
static const char *PA_034 = "Token encoding exception";
static const char *PA_035 = "Token length exception";
static const char *PA_036 = "Insufficient entropy exception";
static const char *PA_255 = "Unknown error";

static inline int get_msb32 (const u32 v)
{
  int i;

  for (i = 32; i > 0; i--) if ((v >> (i - 1)) & 1) break;

  return i;
}

static inline int get_msb64 (const u64 v)
{
  int i;

  for (i = 64; i > 0; i--) if ((v >> (i - 1)) & 1) break;

  return i;
}

bool overflow_check_u32_add (const u32 a, const u32 b)
{
  const int a_msb = get_msb32 (a);
  const int b_msb = get_msb32 (b);

  return ((a_msb < 32) && (b_msb < 32));
}

bool overflow_check_u32_mul (const u32 a, const u32 b)
{
  const int a_msb = get_msb32 (a);
  const int b_msb = get_msb32 (b);

  return ((a_msb + b_msb) < 32);
}

bool overflow_check_u64_add (const u64 a, const u64 b)
{
  const int a_msb = get_msb64 (a);
  const int b_msb = get_msb64 (b);

  return ((a_msb < 64) && (b_msb < 64));
}

bool overflow_check_u64_mul (const u64 a, const u64 b)
{
  const int a_msb = get_msb64 (a);
  const int b_msb = get_msb64 (b);

  return ((a_msb + b_msb) < 64);
}

bool is_power_of_2 (const u32 v)
{
  return (v && !(v & (v - 1)));
}

u32 mydivc32 (const u32 dividend, const u32 divisor)
{
  u32 quotient = dividend / divisor;

  if (dividend % divisor) quotient++;

  return quotient;
}

u64 mydivc64 (const u64 dividend, const u64 divisor)
{
  u64 quotient = dividend / divisor;

  if (dividend % divisor) quotient++;

  return quotient;
}

char *filename_from_filepath (char *filepath)
{
  char *ptr = NULL;

  if ((ptr = strrchr (filepath, '/')) != NULL)
  {
    ptr++;
  }
  else if ((ptr = strrchr (filepath, '\\')) != NULL)
  {
    ptr++;
  }
  else
  {
    ptr = filepath;
  }

  return ptr;
}

void naive_replace (char *s, const char key_char, const char replace_char)
{
  const size_t len = strlen (s);

  for (size_t in = 0; in < len; in++)
  {
    const char c = s[in];

    if (c == key_char)
    {
      s[in] = replace_char;
    }
  }
}

void naive_escape (char *s, size_t s_max, const char key_char, const char escape_char)
{
  char s_escaped[1024] = { 0 };

  size_t s_escaped_max = sizeof (s_escaped);

  const size_t len = strlen (s);

  for (size_t in = 0, out = 0; in < len; in++, out++)
  {
    const char c = s[in];

    if (c == key_char)
    {
      s_escaped[out] = escape_char;

      out++;
    }

    if (out == s_escaped_max - 2) break;

    s_escaped[out] = c;
  }

  strncpy (s, s_escaped, s_max - 1);
}

void hc_asprintf (char **strp, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int rc __attribute__((unused));
  rc = vasprintf (strp, fmt, args);
  va_end (args);
}

#if defined (_WIN)
#define __WINDOWS__
#endif
#include "sort_r.h"
#if defined (_WIN)
#undef __WINDOWS__
#endif

void hc_qsort_r (void *base, size_t nmemb, size_t size, int (*compar) (const void *, const void *, void *), void *arg)
{
  sort_r (base, nmemb, size, compar, arg);
}

void *hc_bsearch_r (const void *key, const void *base, size_t nmemb, size_t size, int (*compar) (const void *, const void *, void *), void *arg)
{
  for (size_t l = 0, r = nmemb; r; r >>= 1)
  {
    const size_t m = r >> 1;

    const size_t c = l + m;

    const char *next = (const char *) base + (c * size);

    const int cmp = (*compar) (key, next, arg);

    if (cmp > 0)
    {
      l += m + 1;

      r--;
    }

    if (cmp == 0) return ((void *) next);
  }

  return (NULL);
}

bool hc_path_is_file (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (S_ISREG (s.st_mode)) return true;

  return false;
}

bool hc_path_is_directory (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (S_ISDIR (s.st_mode)) return true;

  return false;
}

bool hc_path_is_empty (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (s.st_size == 0) return true;

  return false;
}

bool hc_path_exist (const char *path)
{
  if (access (path, F_OK) == -1) return false;

  return true;
}

bool hc_path_read (const char *path)
{
  if (access (path, R_OK) == -1) return false;

  return true;
}

bool hc_path_write (const char *path)
{
  if (access (path, W_OK) == -1) return false;

  return true;
}

bool hc_path_create (const char *path)
{
  if (hc_path_exist (path) == true) return false;

  const int fd = creat (path, S_IRUSR | S_IWUSR);

  if (fd == -1) return false;

  close (fd);

  unlink (path);

  return true;
}

bool hc_path_has_bom (const char *path)
{
  u8 buf[8] = { 0 };

  FILE *fp = fopen (path, "rb");

  if (fp == NULL) return false;

  const size_t nread = fread (buf, 1, sizeof (buf), fp);

  fclose (fp);

  if (nread < 1) return false;

  /* signatures from https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding */

  // utf-8

  if ((buf[0] == 0xef)
   && (buf[1] == 0xbb)
   && (buf[2] == 0xbf)) return true;

  // utf-16

  if ((buf[0] == 0xfe)
   && (buf[1] == 0xff)) return true;

  if ((buf[0] == 0xff)
   && (buf[1] == 0xfe)) return true;

  // utf-32

  if ((buf[0] == 0x00)
   && (buf[1] == 0x00)
   && (buf[2] == 0xfe)
   && (buf[3] == 0xff)) return true;

  if ((buf[0] == 0xff)
   && (buf[1] == 0xfe)
   && (buf[2] == 0x00)
   && (buf[3] == 0x00)) return true;

  // utf-7

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x38)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x39)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x2b)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x2f)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x38)
   && (buf[4] == 0x2d)) return true;

  // utf-1

  if ((buf[0] == 0xf7)
   && (buf[1] == 0x64)
   && (buf[2] == 0x4c)) return true;

  // utf-ebcdic

  if ((buf[0] == 0xdd)
   && (buf[1] == 0x73)
   && (buf[2] == 0x66)
   && (buf[3] == 0x73)) return true;

  // scsu

  if ((buf[0] == 0x0e)
   && (buf[1] == 0xfe)
   && (buf[2] == 0xff)) return true;

  // bocu-1

  if ((buf[0] == 0xfb)
   && (buf[1] == 0xee)
   && (buf[2] == 0x28)) return true;

  // gb-18030

  if ((buf[0] == 0x84)
   && (buf[1] == 0x31)
   && (buf[2] == 0x95)
   && (buf[3] == 0x33)) return true;

  return false;
}

bool hc_string_is_digit (const char *s)
{
  if (s == NULL) return false;

  const size_t len = strlen (s);

  if (len == 0) return false;

  for (size_t i = 0; i < len; i++)
  {
    const int c = (const int) s[i];

    if (isdigit (c) == 0) return false;
  }

  return true;
}

void setup_environment_variables ()
{
  char *compute = getenv ("COMPUTE");

  if (compute)
  {
    char *display;

    hc_asprintf (&display, "DISPLAY=%s", compute);

    putenv (display);

    free (display);
  }
  else
  {
    if (getenv ("DISPLAY") == NULL)
      putenv ((char *) "DISPLAY=:0");
  }

  if (getenv ("OCL_CODE_CACHE_ENABLE") == NULL)
    putenv ((char *) "OCL_CODE_CACHE_ENABLE=0");

  if (getenv ("CUDA_CACHE_DISABLE") == NULL)
    putenv ((char *) "CUDA_CACHE_DISABLE=1");

  if (getenv ("POCL_KERNEL_CACHE") == NULL)
    putenv ((char *) "POCL_KERNEL_CACHE=0");

  if (getenv ("CL_CONFIG_USE_VECTORIZER") == NULL)
    putenv ((char *) "CL_CONFIG_USE_VECTORIZER=False");

  #if defined (__CYGWIN__)
  cygwin_internal (CW_SYNC_WINENV);
  #endif
}

void setup_umask ()
{
  umask (077);
}

void setup_seeding (const bool rp_gen_seed_chgd, const u32 rp_gen_seed)
{
  if (rp_gen_seed_chgd == true)
  {
    srand (rp_gen_seed);
  }
  else
  {
    const time_t ts = time (NULL); // don't tell me that this is an insecure seed

    srand ((unsigned int) ts);
  }
}

u32 get_random_num (const u32 min, const u32 max)
{
  if (min == max) return (min);

  const u32 low = max - min;

  if (low == 0) return (0);

  #if defined (_WIN)

  return (((u32) rand () % (max - min)) + min);

  #else

  return (((u32) random () % (max - min)) + min);

  #endif
}

void hc_string_trim_leading (char *s)
{
  int skip = 0;

  const int len = (int) strlen (s);

  for (int i = 0; i < len; i++)
  {
    const int c = (const int) s[i];

    if (isspace (c) == 0) break;

    skip++;
  }

  if (skip == 0) return;

  const int new_len = len - skip;

  memmove (s, s + skip, new_len);

  s[new_len] = 0;
}

void hc_string_trim_trailing (char *s)
{
  int skip = 0;

  const int len = (int) strlen (s);

  for (int i = len - 1; i >= 0; i--)
  {
    const int c = (const int) s[i];

    if (isspace (c) == 0) break;

    skip++;
  }

  if (skip == 0) return;

  const size_t new_len = len - skip;

  s[new_len] = 0;
}

bool hc_fopen (fp_tmp_t *fp_t, const char *path, char *mode)
{
  unsigned char check[3] = { 0 };

  FILE *fp = fopen (path, mode);

  if (fp == NULL) return false;

  check[0] = fgetc (fp);

  check[1] = fgetc (fp);

  check[2] = fgetc (fp);

  fp_t->is_gzip = -1;

  if (check[0] == 0x1f && check[1] == 0x8b && check[2] == 0x08)
  {
    fclose (fp);

    if (!(fp_t->f.gfp = gzopen (path, mode))) return false;

    fp_t->is_gzip = 1;

  }
  else
  {
    fp_t->f.fp = fp;

    rewind (fp_t->f.fp);

    fp_t->is_gzip = 0;
  }

  fp_t->path = path;

  fp_t->mode = mode;

  return true;
}

size_t hc_fread (void *ptr, size_t size, size_t nmemb, fp_tmp_t *fp_t)
{
  size_t n = 0;

  if (fp_t == NULL) return -1;

  if (fp_t->is_gzip)
    n = gzfread (ptr, size, nmemb, fp_t->f.gfp);
  else
    n = fread (ptr, size, nmemb, fp_t->f.fp);

  return n;
}

size_t hc_fwrite (void *ptr, size_t size, size_t nmemb, fp_tmp_t *fp_t)
{
  size_t n = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    n = gzfwrite (ptr, size, nmemb, fp_t->f.gfp);
  else
    n = fwrite (ptr, size, nmemb, fp_t->f.fp);

  if (n != nmemb) return -1;

  return n;
}

size_t hc_fread_direct (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fread (ptr, size, nmemb, stream);
}

void hc_fwrite_direct (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t rc = fwrite (ptr, size, nmemb, stream);

  if (rc == 0) rc = 0;
}

int hc_fseek (fp_tmp_t *fp_t, off_t offset, int whence)
{
  int r = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    r = gzseek (fp_t->f.gfp, (z_off_t) offset, whence);
  else
    r = fseeko (fp_t->f.fp, offset, whence);

  return r;
}

void hc_rewind (fp_tmp_t *fp_t)
{
  if (fp_t == NULL || fp_t->is_gzip == -1) return;

  if (fp_t->is_gzip)
    gzrewind (fp_t->f.gfp);
  else
    rewind (fp_t->f.fp);
}

off_t hc_ftell (fp_tmp_t *fp_t)
{
  off_t n = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    n = (off_t) gztell (fp_t->f.gfp);
  else
    n = ftello (fp_t->f.fp);

  return n;
}

int hc_fputc (int c, fp_tmp_t *fp_t)
{
  int r = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    r = gzputc (fp_t->f.gfp, c);
  else
    r = fputc (c, fp_t->f.fp);

  return r;
}

int hc_fgetc (fp_tmp_t *fp_t)
{
  int r = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    r = gzgetc (fp_t->f.gfp);
  else
    r = fgetc (fp_t->f.fp);

  return r;
}

char *hc_fgets (char *buf, int len, fp_tmp_t *fp_t)
{
  char *r = NULL;

  if (fp_t == NULL || fp_t->is_gzip == -1) return NULL;

  if (fp_t->is_gzip)
    r = gzgets (fp_t->f.gfp, buf, len);
  else
    r = fgets (buf, len, fp_t->f.fp);

  return r;
}

int hc_fileno (fp_tmp_t *fp_t)
{
  int r = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
  {
    int rdup = fileno (fopen (fp_t->path, fp_t->mode));

    r = dup(rdup);

    close(rdup);
  }
  else
    r = fileno (fp_t->f.fp);

  return r;
}

int hc_feof (fp_tmp_t *fp_t)
{
  int r = 0;

  if (fp_t == NULL || fp_t->is_gzip == -1) return -1;

  if (fp_t->is_gzip)
    r = gzeof (fp_t->f.gfp);
  else
    r = feof (fp_t->f.fp);

  return r;
}

/*
void hc_fflush (fp_tmp_t *fp_t)
{
  if (fp_t == NULL || fp_t->is_gzip == -1) return;

  if (fp_t->is_gzip)
    gzflush (fp_t->f.gfp, Z_SYNC_FLUSH);
	else
    fflush (fp_t->f.fp);
}
*/

void hc_fclose (fp_tmp_t *fp_t)
{
  if (fp_t == NULL || fp_t->is_gzip == -1) return;

  if (fp_t->is_gzip)
    gzclose (fp_t->f.gfp);
  else
    fclose (fp_t->f.fp);

  fp_t->is_gzip = -1;

  fp_t->path = NULL;

  fp_t->mode = NULL;
}

bool hc_same_files (char *file1, char *file2)
{
  if ((file1 != NULL) && (file2 != NULL))
  {
    struct stat tmpstat_file1;
    struct stat tmpstat_file2;

    int do_check = 0;

    FILE *fp;

    fp = fopen (file1, "r");

    if (fp)
    {
      if (fstat (fileno (fp), &tmpstat_file1))
      {
        fclose (fp);

        return false;
      }

      fclose (fp);

      do_check++;
    }

    fp = fopen (file2, "r");

    if (fp)
    {
      if (fstat (fileno (fp), &tmpstat_file2))
      {
        fclose (fp);

        return false;
      }

      fclose (fp);

      do_check++;
    }

    if (do_check == 2)
    {
      tmpstat_file1.st_mode     = 0;
      tmpstat_file1.st_nlink    = 0;
      tmpstat_file1.st_uid      = 0;
      tmpstat_file1.st_gid      = 0;
      tmpstat_file1.st_rdev     = 0;
      tmpstat_file1.st_atime    = 0;

      #if defined (STAT_NANOSECONDS_ACCESS_TIME)
      tmpstat_file1.STAT_NANOSECONDS_ACCESS_TIME = 0;
      #endif

      #if defined (_POSIX)
      tmpstat_file1.st_blksize  = 0;
      tmpstat_file1.st_blocks   = 0;
      #endif

      tmpstat_file2.st_mode     = 0;
      tmpstat_file2.st_nlink    = 0;
      tmpstat_file2.st_uid      = 0;
      tmpstat_file2.st_gid      = 0;
      tmpstat_file2.st_rdev     = 0;
      tmpstat_file2.st_atime    = 0;

      #if defined (STAT_NANOSECONDS_ACCESS_TIME)
      tmpstat_file2.STAT_NANOSECONDS_ACCESS_TIME = 0;
      #endif

      #if defined (_POSIX)
      tmpstat_file2.st_blksize  = 0;
      tmpstat_file2.st_blocks   = 0;
      #endif

      if (memcmp (&tmpstat_file1, &tmpstat_file2, sizeof (struct stat)) == 0)
      {
        return true;
      }
    }
  }

  return false;
}

u32 hc_strtoul (const char *nptr, char **endptr, int base)
{
  return (u32) strtoul (nptr, endptr, base);
}

u64 hc_strtoull (const char *nptr, char **endptr, int base)
{
  return (u64) strtoull (nptr, endptr, base);
}

u32 power_of_two_ceil_32 (const u32 v)
{
  u32 r = v;

  r--;

  r |= r >> 1;
  r |= r >> 2;
  r |= r >> 4;
  r |= r >> 8;
  r |= r >> 16;

  r++;

  return r;
}

u32 power_of_two_floor_32 (const u32 v)
{
  u32 r = power_of_two_ceil_32 (v);

  if (r > v)
  {
    r >>= 1;
  }

  return r;
}

u32 round_up_multiple_32 (const u32 v, const u32 m)
{
  if (m == 0) return v;

  const u32 r = v % m;

  if (r == 0) return v;

  return v + m - r;
}

u64 round_up_multiple_64 (const u64 v, const u64 m)
{
  if (m == 0) return v;

  const u64 r = v % m;

  if (r == 0) return v;

  return v + m - r;
}

// difference to original strncat is no returncode and u8* instead of char*

void hc_strncat (u8 *dst, const u8 *src, const size_t n)
{
  const size_t dst_len = strlen ((char *) dst);

  const u8 *src_ptr = src;

  u8 *dst_ptr = dst + dst_len;

  for (size_t i = 0; i < n && *src_ptr != 0; i++)
  {
    *dst_ptr++ = *src_ptr++;
  }

  *dst_ptr = 0;
}

int count_char (const u8 *buf, const int len, const u8 c)
{
  int r = 0;

  for (int i = 0; i < len; i++)
  {
    if (buf[i] == c) r++;
  }

  return r;
}

float get_entropy (const u8 *buf, const int len)
{
  float entropy = 0.0;

  for (int c = 0; c < 256; c++)
  {
    const int r = count_char (buf, len, (const u8) c);

    if (r == 0) continue;

    float w = (float) r / len;

    entropy += -w * log2 (w);
  }

  return entropy;
}

#if defined (_WIN)

#else

int select_read_timeout (int sockfd, const int sec)
{
  struct timeval tv;

  tv.tv_sec  = sec;
  tv.tv_usec = 0;

  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (sockfd, &fds);

  return select (sockfd + 1, &fds, NULL, NULL, &tv);
}

int select_write_timeout (int sockfd, const int sec)
{
  struct timeval tv;

  tv.tv_sec  = sec;
  tv.tv_usec = 0;

  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (sockfd, &fds);

  return select (sockfd + 1, NULL, &fds, NULL, &tv);
}

#endif

#if defined (_WIN)

int select_read_timeout_console (const int sec)
{
  const HANDLE hStdIn = GetStdHandle (STD_INPUT_HANDLE);

  const DWORD rc = WaitForSingleObject (hStdIn, sec * 1000);

  if (rc == WAIT_OBJECT_0)
  {
    DWORD dwRead;

    INPUT_RECORD inRecords;

    inRecords.EventType = 0;

    PeekConsoleInput (hStdIn, &inRecords, 1, &dwRead);

    if (inRecords.EventType == 0)
    {
      // those are good ones

      return 1;
    }
    else
    {
      // but we don't want that stuff like windows focus etc. in our stream

      ReadConsoleInput (hStdIn, &inRecords, 1, &dwRead);
    }

    return select_read_timeout_console (sec);
  }
  else if (rc == WAIT_TIMEOUT)
  {
    return 0;
  }

  return -1;
}

#else

int select_read_timeout_console (const int sec)
{
  return select_read_timeout (fileno (stdin), sec);
}

#endif

const char *strparser (const u32 parser_status)
{
  switch (parser_status)
  {
    case PARSER_OK:                   return PA_000;
    case PARSER_COMMENT:              return PA_001;
    case PARSER_GLOBAL_ZERO:          return PA_002;
    case PARSER_GLOBAL_LENGTH:        return PA_003;
    case PARSER_HASH_LENGTH:          return PA_004;
    case PARSER_HASH_VALUE:           return PA_005;
    case PARSER_SALT_LENGTH:          return PA_006;
    case PARSER_SALT_VALUE:           return PA_007;
    case PARSER_SALT_ITERATION:       return PA_008;
    case PARSER_SEPARATOR_UNMATCHED:  return PA_009;
    case PARSER_SIGNATURE_UNMATCHED:  return PA_010;
    case PARSER_HCCAPX_FILE_SIZE:     return PA_011;
    case PARSER_HCCAPX_EAPOL_LEN:     return PA_012;
    case PARSER_PSAFE2_FILE_SIZE:     return PA_013;
    case PARSER_PSAFE3_FILE_SIZE:     return PA_014;
    case PARSER_TC_FILE_SIZE:         return PA_015;
    case PARSER_VC_FILE_SIZE:         return PA_016;
    case PARSER_SIP_AUTH_DIRECTIVE:   return PA_017;
    case PARSER_HASH_FILE:            return PA_018;
    case PARSER_HASH_ENCODING:        return PA_019;
    case PARSER_SALT_ENCODING:        return PA_020;
    case PARSER_LUKS_FILE_SIZE:       return PA_021;
    case PARSER_LUKS_MAGIC:           return PA_022;
    case PARSER_LUKS_VERSION:         return PA_023;
    case PARSER_LUKS_CIPHER_TYPE:     return PA_024;
    case PARSER_LUKS_CIPHER_MODE:     return PA_025;
    case PARSER_LUKS_HASH_TYPE:       return PA_026;
    case PARSER_LUKS_KEY_SIZE:        return PA_027;
    case PARSER_LUKS_KEY_DISABLED:    return PA_028;
    case PARSER_LUKS_KEY_STRIPES:     return PA_029;
    case PARSER_LUKS_HASH_CIPHER:     return PA_030;
    case PARSER_HCCAPX_SIGNATURE:     return PA_031;
    case PARSER_HCCAPX_VERSION:       return PA_032;
    case PARSER_HCCAPX_MESSAGE_PAIR:  return PA_033;
    case PARSER_TOKEN_ENCODING:       return PA_034;
    case PARSER_TOKEN_LENGTH:         return PA_035;
    case PARSER_INSUFFICIENT_ENTROPY: return PA_036;
  }

  return PA_255;
}

static int rounds_count_length (const char *input_buf, const int input_len)
{
  if (input_len >= 9) // 9 is minimum because of "rounds=X$"
  {
    static const char *rounds = "rounds=";

    if (memcmp (input_buf, rounds, 7) == 0)
    {
      char *next_pos = strchr (input_buf + 8, '$');

      if (next_pos == NULL) return -1;

      const int rounds_len = next_pos - input_buf;

      return rounds_len;
    }
  }

  return -1;
}

int input_tokenizer (const u8 *input_buf, const int input_len, token_t *token)
{
  int len_left = input_len;

  token->buf[0] = input_buf;

  int token_idx;

  for (token_idx = 0; token_idx < token->token_cnt - 1; token_idx++)
  {
    if (token->attr[token_idx] & TOKEN_ATTR_FIXED_LENGTH)
    {
      int len = token->len[token_idx];

      if (len_left < len) return (PARSER_TOKEN_LENGTH);

      token->buf[token_idx + 1] = token->buf[token_idx] + len;

      len_left -= len;
    }
    else
    {
      if (token->attr[token_idx] & TOKEN_ATTR_OPTIONAL_ROUNDS)
      {
        const int len = rounds_count_length ((const char *) token->buf[token_idx], len_left);

        token->opt_buf = token->buf[token_idx];

        token->opt_len = len; // we want an eventual -1 in here, it's used later for verification

        if (len > 0)
        {
          token->buf[token_idx] += len + 1; // +1 = separator

          len_left -= len + 1; // +1 = separator
        }
      }

      const u8 *next_pos = (const u8 *) strchr ((const char *) token->buf[token_idx], token->sep[token_idx]);

      if (next_pos == NULL) return (PARSER_SEPARATOR_UNMATCHED);

      const int len = next_pos - token->buf[token_idx];

      token->len[token_idx] = len;

      token->buf[token_idx + 1] = next_pos + 1; // +1 = separator

      len_left -= len + 1; // +1 = separator
    }
  }

  if (token->attr[token_idx] & TOKEN_ATTR_FIXED_LENGTH)
  {
    int len = token->len[token_idx];

    if (len_left != len) return (PARSER_TOKEN_LENGTH);
  }
  else
  {
    token->len[token_idx] = len_left;
  }

  // verify data

  for (token_idx = 0; token_idx < token->token_cnt; token_idx++)
  {
    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_SIGNATURE)
    {
      bool matched = false;

      for (int signature_idx = 0; signature_idx < token->signatures_cnt; signature_idx++)
      {
        if (memcmp (token->buf[token_idx], token->signatures_buf[signature_idx], token->len[token_idx]) == 0) matched = true;
      }

      if (matched == false) return (PARSER_SIGNATURE_UNMATCHED);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_LENGTH)
    {
      if (token->len[token_idx] < token->len_min[token_idx]) return (PARSER_TOKEN_LENGTH);
      if (token->len[token_idx] > token->len_max[token_idx]) return (PARSER_TOKEN_LENGTH);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_HEX)
    {
      if (is_valid_hex_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64A)
    {
      if (is_valid_base64a_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64B)
    {
      if (is_valid_base64b_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64C)
    {
      if (is_valid_base64c_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }
  }

  return PARSER_OK;
}

void decoder_apply_optimizer (const hashconfig_t *hashconfig, void *data)
{
  const u32 hash_type = hashconfig->hash_type;
  const u32 opti_type = hashconfig->opti_type;

  u32 *digest_buf   = (u32 *) data;
  u64 *digest_buf64 = (u64 *) data;

  if (opti_type & OPTI_TYPE_PRECOMPUTE_PERMUT)
  {
    u32 tt;

    switch (hash_type)
    {
      case HASH_TYPE_DES:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_DESCRYPT:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_DESRACF:
        digest_buf[0] = rotl32 (digest_buf[0], 29);
        digest_buf[1] = rotl32 (digest_buf[1], 29);

        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_LM:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_NETNTLM:
        digest_buf[0] = rotl32 (digest_buf[0], 29);
        digest_buf[1] = rotl32 (digest_buf[1], 29);
        digest_buf[2] = rotl32 (digest_buf[2], 29);
        digest_buf[3] = rotl32 (digest_buf[3], 29);

        FP (digest_buf[1], digest_buf[0], tt);
        FP (digest_buf[3], digest_buf[2], tt);
        break;

      case HASH_TYPE_BSDICRYPT:
        digest_buf[0] = rotl32 (digest_buf[0], 31);
        digest_buf[1] = rotl32 (digest_buf[1], 31);

        FP (digest_buf[1], digest_buf[0], tt);
        break;
    }
  }

  if (opti_type & OPTI_TYPE_PRECOMPUTE_MERKLE)
  {
    switch (hash_type)
    {
      case HASH_TYPE_MD4:
        digest_buf[0] -= MD4M_A;
        digest_buf[1] -= MD4M_B;
        digest_buf[2] -= MD4M_C;
        digest_buf[3] -= MD4M_D;
        break;

      case HASH_TYPE_MD5:
        digest_buf[0] -= MD5M_A;
        digest_buf[1] -= MD5M_B;
        digest_buf[2] -= MD5M_C;
        digest_buf[3] -= MD5M_D;
        break;

      case HASH_TYPE_SHA1:
        digest_buf[0] -= SHA1M_A;
        digest_buf[1] -= SHA1M_B;
        digest_buf[2] -= SHA1M_C;
        digest_buf[3] -= SHA1M_D;
        digest_buf[4] -= SHA1M_E;
        break;

      case HASH_TYPE_SHA224:
        digest_buf[0] -= SHA224M_A;
        digest_buf[1] -= SHA224M_B;
        digest_buf[2] -= SHA224M_C;
        digest_buf[3] -= SHA224M_D;
        digest_buf[4] -= SHA224M_E;
        digest_buf[5] -= SHA224M_F;
        digest_buf[6] -= SHA224M_G;
        break;

      case HASH_TYPE_SHA256:
        digest_buf[0] -= SHA256M_A;
        digest_buf[1] -= SHA256M_B;
        digest_buf[2] -= SHA256M_C;
        digest_buf[3] -= SHA256M_D;
        digest_buf[4] -= SHA256M_E;
        digest_buf[5] -= SHA256M_F;
        digest_buf[6] -= SHA256M_G;
        digest_buf[7] -= SHA256M_H;
        break;

      case HASH_TYPE_SHA384:
        digest_buf64[0] -= SHA384M_A;
        digest_buf64[1] -= SHA384M_B;
        digest_buf64[2] -= SHA384M_C;
        digest_buf64[3] -= SHA384M_D;
        digest_buf64[4] -= SHA384M_E;
        digest_buf64[5] -= SHA384M_F;
        digest_buf64[6] -= 0;
        digest_buf64[7] -= 0;
        break;

      case HASH_TYPE_SHA512:
        digest_buf64[0] -= SHA512M_A;
        digest_buf64[1] -= SHA512M_B;
        digest_buf64[2] -= SHA512M_C;
        digest_buf64[3] -= SHA512M_D;
        digest_buf64[4] -= SHA512M_E;
        digest_buf64[5] -= SHA512M_F;
        digest_buf64[6] -= SHA512M_G;
        digest_buf64[7] -= SHA512M_H;
        break;
    }
  }
}

void encoder_apply_optimizer (const hashconfig_t *hashconfig, void *data)
{
  const u32 hash_type = hashconfig->hash_type;
  const u32 opti_type = hashconfig->opti_type;

  u32 *digest_buf   = (u32 *) data;
  u64 *digest_buf64 = (u64 *) data;

  if (opti_type & OPTI_TYPE_PRECOMPUTE_PERMUT)
  {
    u32 tt;

    switch (hash_type)
    {
      case HASH_TYPE_DES:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_DESCRYPT:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_DESRACF:
        digest_buf[0] = rotl32 (digest_buf[0], 29);
        digest_buf[1] = rotl32 (digest_buf[1], 29);

        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_LM:
        FP (digest_buf[1], digest_buf[0], tt);
        break;

      case HASH_TYPE_NETNTLM:
        digest_buf[0] = rotl32 (digest_buf[0], 29);
        digest_buf[1] = rotl32 (digest_buf[1], 29);
        digest_buf[2] = rotl32 (digest_buf[2], 29);
        digest_buf[3] = rotl32 (digest_buf[3], 29);

        FP (digest_buf[1], digest_buf[0], tt);
        FP (digest_buf[3], digest_buf[2], tt);
        break;

      case HASH_TYPE_BSDICRYPT:
        digest_buf[0] = rotl32 (digest_buf[0], 31);
        digest_buf[1] = rotl32 (digest_buf[1], 31);

        FP (digest_buf[1], digest_buf[0], tt);
        break;
    }
  }

  if (opti_type & OPTI_TYPE_PRECOMPUTE_MERKLE)
  {
    switch (hash_type)
    {
      case HASH_TYPE_MD4:
        digest_buf[0] += MD4M_A;
        digest_buf[1] += MD4M_B;
        digest_buf[2] += MD4M_C;
        digest_buf[3] += MD4M_D;
        break;

      case HASH_TYPE_MD5:
        digest_buf[0] += MD5M_A;
        digest_buf[1] += MD5M_B;
        digest_buf[2] += MD5M_C;
        digest_buf[3] += MD5M_D;
        break;

      case HASH_TYPE_SHA1:
        digest_buf[0] += SHA1M_A;
        digest_buf[1] += SHA1M_B;
        digest_buf[2] += SHA1M_C;
        digest_buf[3] += SHA1M_D;
        digest_buf[4] += SHA1M_E;
        break;

      case HASH_TYPE_SHA224:
        digest_buf[0] += SHA224M_A;
        digest_buf[1] += SHA224M_B;
        digest_buf[2] += SHA224M_C;
        digest_buf[3] += SHA224M_D;
        digest_buf[4] += SHA224M_E;
        digest_buf[5] += SHA224M_F;
        digest_buf[6] += SHA224M_G;
        break;

      case HASH_TYPE_SHA256:
        digest_buf[0] += SHA256M_A;
        digest_buf[1] += SHA256M_B;
        digest_buf[2] += SHA256M_C;
        digest_buf[3] += SHA256M_D;
        digest_buf[4] += SHA256M_E;
        digest_buf[5] += SHA256M_F;
        digest_buf[6] += SHA256M_G;
        digest_buf[7] += SHA256M_H;
        break;

      case HASH_TYPE_SHA384:
        digest_buf64[0] += SHA384M_A;
        digest_buf64[1] += SHA384M_B;
        digest_buf64[2] += SHA384M_C;
        digest_buf64[3] += SHA384M_D;
        digest_buf64[4] += SHA384M_E;
        digest_buf64[5] += SHA384M_F;
        digest_buf64[6] += 0;
        digest_buf64[7] += 0;
        break;

      case HASH_TYPE_SHA512:
        digest_buf64[0] += SHA512M_A;
        digest_buf64[1] += SHA512M_B;
        digest_buf64[2] += SHA512M_C;
        digest_buf64[3] += SHA512M_D;
        digest_buf64[4] += SHA512M_E;
        digest_buf64[5] += SHA512M_F;
        digest_buf64[6] += SHA512M_G;
        digest_buf64[7] += SHA512M_H;
        break;
    }
  }
}

void decoder_apply_options (const hashconfig_t *hashconfig, void *data)
{
  const u32 hash_type = hashconfig->hash_type;
  const u64 opts_type = hashconfig->opts_type;
  const u32 dgst_size = hashconfig->dgst_size;

  u32 *digest_buf   = (u32 *) data;
  u64 *digest_buf64 = (u64 *) data;

  if (opts_type & OPTS_TYPE_STATE_BUFFER_BE)
  {
    if (dgst_size == DGST_SIZE_4_2)
    {
      for (int i = 0; i < 2; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_4)
    {
      for (int i = 0; i < 4; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_5)
    {
      for (int i = 0; i < 5; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_6)
    {
      for (int i = 0; i < 6; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_7)
    {
      for (int i = 0; i < 7; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_8)
    {
      for (int i = 0; i < 8; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if ((dgst_size == DGST_SIZE_4_16) || (dgst_size == DGST_SIZE_8_8)) // same size, same result :)
    {
      if (hash_type == HASH_TYPE_WHIRLPOOL)
      {
        for (int i = 0; i < 16; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
      }
      else if (hash_type == HASH_TYPE_SHA384)
      {
        for (int i = 0; i < 8; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
      }
      else if (hash_type == HASH_TYPE_SHA512)
      {
        for (int i = 0; i < 8; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
      }
      else if (hash_type == HASH_TYPE_GOST)
      {
        for (int i = 0; i < 16; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
      }
    }
    else if (dgst_size == DGST_SIZE_4_64)
    {
      for (int i = 0; i < 64; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_8_25)
    {
      for (int i = 0; i < 25; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
    }
  }
}

void encoder_apply_options (const hashconfig_t *hashconfig, void *data)
{
  const u32 hash_type = hashconfig->hash_type;
  const u64 opts_type = hashconfig->opts_type;
  const u32 dgst_size = hashconfig->dgst_size;

  u32 *digest_buf   = (u32 *) data;
  u64 *digest_buf64 = (u64 *) data;

  if (opts_type & OPTS_TYPE_STATE_BUFFER_BE)
  {
    if (dgst_size == DGST_SIZE_4_2)
    {
      for (int i = 0; i < 2; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_4)
    {
      for (int i = 0; i < 4; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_5)
    {
      for (int i = 0; i < 5; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_6)
    {
      for (int i = 0; i < 6; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_7)
    {
      for (int i = 0; i < 7; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_4_8)
    {
      for (int i = 0; i < 8; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if ((dgst_size == DGST_SIZE_4_16) || (dgst_size == DGST_SIZE_8_8)) // same size, same result :)
    {
      if (hash_type == HASH_TYPE_WHIRLPOOL)
      {
        for (int i = 0; i < 16; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
      }
      else if (hash_type == HASH_TYPE_SHA384)
      {
        for (int i = 0; i < 8; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
      }
      else if (hash_type == HASH_TYPE_SHA512)
      {
        for (int i = 0; i < 8; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
      }
      else if (hash_type == HASH_TYPE_GOST)
      {
        for (int i = 0; i < 16; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
      }
    }
    else if (dgst_size == DGST_SIZE_4_64)
    {
      for (int i = 0; i < 64; i++) digest_buf[i] = byte_swap_32 (digest_buf[i]);
    }
    else if (dgst_size == DGST_SIZE_8_25)
    {
      for (int i = 0; i < 25; i++) digest_buf64[i] = byte_swap_64 (digest_buf64[i]);
    }
  }
}
