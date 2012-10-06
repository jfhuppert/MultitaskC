# Makefile

# BINARIES=boom16_r boom8_r boom4_c boom4_r boom2_c boom2_r
BINARIES=boom16_c boom16_r boom8_c boom8_r boom4_c boom4_r boom2_c boom2_r

all : $(BINARIES)
	@echo done

boom16_c : boom16_c.c
	-time gcc -O2 boom16_c.c -o boom16_c

boom16_c.c : boom16.mtc
	-time mtcc -c boom16.mtc boom16_c.c

boom16_r : boom16_r.c
	-time gcc -O2 boom16_r.c -o boom16_r

boom16_r.c : boom16.mtc
	-time mtcc -e boom16.mtc boom16_r.c

boom8_c : boom8_c.c
	-time gcc -O2 boom8_c.c -o boom8_c

boom8_c.c : boom8.mtc
	-time mtcc -c boom8.mtc boom8_c.c

boom8_r : boom8_r.c
	-time gcc -O2 boom8_r.c -o boom8_r

boom8_r.c : boom8.mtc
	-time mtcc -e boom8.mtc boom8_r.c

boom4_c : boom4_c.c
	-time gcc -O2 boom4_c.c -o boom4_c

boom4_c.c : boom4.mtc
	-time mtcc -c boom4.mtc boom4_c.c

boom4_r : boom4_r.c
	-time gcc -O2 boom4_r.c -o boom4_r

boom4_r.c : boom4.mtc
	-time mtcc -e boom4.mtc boom4_r.c

boom2_c : boom2_c.c
	-time gcc -O2 boom2_c.c -o boom2_c

boom2_c.c : boom2.mtc
	-time mtcc -c boom2.mtc boom2_c.c

boom2_r : boom2_r.c
	-time gcc -O2 boom2_r.c -o boom2_r

boom2_r.c : boom2.mtc
	-time mtcc -e boom2.mtc boom2_r.c

clean :
	rm -f $(BINARIES) *.c *.PmT *.log
