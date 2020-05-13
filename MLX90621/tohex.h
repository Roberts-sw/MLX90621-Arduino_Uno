#ifdef __cplusplus
#	define CCALL	extern "C"	//C-call conventions
#else
#	define CCALL
#endif

CCALL char const *tohex(unsigned char ch);
