# c/n2/Makefile 4/25/91
# Expects $(OUTPUT) to define desired output writer.

CC2=\
	cc2.$X\
	cc2sym.$X\
	dbgt2.$X\
	emit0.$X\
	getfun.$X\
	optim.$X

CC2MCH=\
	afield.$X\
	asm.$X\
	emit1.$X\
	getcod.$X\
	optab.$X\
	peep.$X\
	unasm.$X

# Primary targets.
cc2:	$(CC2) $(CC2MCH) $(OUTPUT) $(CLIB)
	$(CC) $(LDFLAGS) -o cc2 $<

# Machine independent.
cc2.$X:		cc2.c
cc2sym.$X:	cc2sym.c
dbgt2.$X:	dbgt2.c
emit0.$X:	emit0.c
getfun.$X:	getfun.c
optim.$X:	optim.c

# Machine dependent.
afield.$X:	afield.c
asm.$X:		asm.c
emit1.$X:	emit1.c
getcod.$X:	getcod.c
optab.$X:	optab.c
peep.$X:	peep.c
unasm.$X:	unasm.c

# Output writers.
# Machine dependent to a great extent, though they shouldn't be.
# out286 - writes Intel 80286 object format
# outcoh - writes COHERENT l.$Xut.h format
# outcsd - writes Intel 8087 object format with csd DEBUG segment
# outrel - writes COHERENT reloc.h format
# outomf - writes Intel 8086 object format
out286.$X:	out286.c
outcoh.$X:	outcoh.c
outcsd.$X:	outcsd.c
outomf.$X:	outomf.c
outrel.$X:	outrel.c

# c/n2/Makefile
