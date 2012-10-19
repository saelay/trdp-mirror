Here are DLLs, for a 32bit Windows installation.


In general:
	pthread[VG]{SE,CE,C}[c].dll
	pthread[VG]{SE,CE,C}[c].lib

where:
	[VG] indicates the compiler
	V	- MS VC, or
	G	- GNU C

	{SE,CE,C} indicates the exception handling scheme
	SE	- Structured EH, or
	CE	- C++ EH, or
	C	- no exceptions - uses setjmp/longjmp

	c	- DLL compatibility number indicating ABI and API
		  compatibility with applications built against
		  a snapshot with the same compatibility number.
		  See 'Version numbering' below.