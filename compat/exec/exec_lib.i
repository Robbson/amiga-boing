	IFND	BOING_COMPAT_EXEC_LIB_I
BOING_COMPAT_EXEC_LIB_I	SET	1
**
**	NDK 3.2 compatibility shim.
**	NDK 3.2's exec/exec_lib.i only invokes an undefined FUNCDEF macro; the
**	actual _LVO* offsets live in lvo/exec_lib.i. Map the old NDK 1.3 path.
**
	INCLUDE	"lvo/exec_lib.i"
	ENDC
