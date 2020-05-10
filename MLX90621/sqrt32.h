#ifdef __cplusplus
#	define CCALL	extern "C"	//C-call conventions
#else
#	define CCALL
#endif

CCALL uint16_t sqrt32(uint32_t x);
