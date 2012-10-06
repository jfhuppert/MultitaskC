#ifndef _SMAPPING_H
#define _SMAPPING_H

extern inline int
strlen(
	const char *s)
{
	register int i;
	for (i=0; s[i]; i++) {}
	return i;
}

extern inline void *
memset(
	void *s,
	int c,
	int n)
{
	while (n--) ((char *)s)[n] = (char)c;
	return s;
}

extern inline void *
memcpy(
	void *dest,
	const void *src,
	int n)
{
	register int i;
	for (i=0; i<n; i++) ((char *)dest)[i] = ((char *)src)[i];
	return dest;
}

extern inline int
memcmp(
	const void *s1,
	const void *s2,
	int n)
{
	register int i;
	register int v;

	for (i=0; i<n; i++) {
		v = ((char *)s1)[i] - ((char *)s2)[i];
		if (v != 0) return v;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
extern inline void
swap_char(
  char *s,
  int p1,
  int p2)
{
  char tmp = s[p1];
  s[p1] = s[p2];
  s[p2] = tmp;
}

/*----------------------------------------------------------------------------*/
extern inline char *
strrev(
  char *s)
{
  register int i, l;

	for (l=0; s[l]; l++) { }
  for (i=0; i<l/2; i++) swap_char(s, i, l-i-1);

  return s;
}

#endif
