/////////
/
/ Multiplication. This is made fairly simple because the multiply can
/ only be done in one register (dxax) and the multiply instruction does
/ not set any flags.
/
/////////

MUL:
%	PEFFECT|PRVALUE|P80186
	WORD		ANYR	*	*	TEMP
		ADR		WORD
		IMM|MMX		WORD
%	PLVALUE|P80186
	WORD		ANYL	*	*	TEMP
		ADR		WORD
		IMM|MMX		WORD
			[ZIMULI]	[R],[AL],[AR]

%	PEFFECT|PRVALUE|P80186
	WORD		ANYR	*	*	TEMP
		IMM|MMX		WORD
		ADR		WORD
%	PLVALUE|P80186
	WORD		ANYL	*	*	TEMP
		IMM|MMX		WORD
		ADR		WORD
			[ZIMULI]	[R],[AR],[AL]

%	PEFFECT|PRVALUE
	WORD		DXAX	AX	*	AX
		TREG		FS16
		ADR		WORD
			[ZIMUL]	[AR]

%	PEFFECT|PRVALUE
	WORD		DXAX	AX	*	AX
		TREG		UWORD
		ADR		WORD
			[ZMUL]	[AR]

////////
/
/ Floating point, using the numeric
/ data coprocessor (8087).
/
////////

#ifdef NDPDEF
%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FS16
			[ZFMULI] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FS32
			[ZFMULL] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FF32
			[ZFMULF] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FF64
			[ZFMULD] [AR]
#endif
