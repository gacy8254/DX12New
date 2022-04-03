
//存放用于shader的一些公用宏定义

//----------------------------------------------------------------------------------------------------------
// Common
//----------------------------------------------------------------------------------------------------------
#define PI 3.141592653589793238462643383279

//----------------------------------------------------------------------------------------------------------
// Reverse Z
//----------------------------------------------------------------------------------------------------------

#define USE_REVERSE_Z 1

#define Z_UPPER_BOUND 50000.0f
#define Z_LOWER_BOUND 1.0f
#define Z_UPPER_BOUND_NORM 1.0f
#define Z_LOWER_BOUND_NORM 0.0f

#if USE_REVERSE_Z

#define FAR_Z Z_LOWER_BOUND
#define NEAR_Z Z_UPPER_BOUND
#define FAR_Z_NORM Z_LOWER_BOUND_NORM
#define NEAR_Z_NORM Z_UPPER_BOUND_NORM

#else

#define FAR_Z Z_UPPER_BOUND
#define NEAR_Z Z_LOWER_BOUND
#define FAR_Z_NORM Z_UPPER_BOUND_NORM
#define NEAR_Z_NORM Z_LOWER_BOUND_NORM

#endif

#define UNREAL_LIGHT_ATTENUATION 0