
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

//使用reverseZ
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

//使用虚幻的衰减公式
#define UNREAL_LIGHT_ATTENUATION 1

//使用TAA
#define USE_TAA 1

#define TAA_SAMPLE_COUNT 8

//// 8x TAA
//static const double Halton_2[8] =
//{
//	0.0,
//	-1.0 / 2.0,
//	1.0 / 2.0,
//	-3.0 / 4.0,
//	1.0 / 4.0,
//	-1.0 / 4.0,
//	3.0 / 4.0,
//	-7.0 / 8.0
//};
//
//// 8x TAA
//static const double Halton_3[8] =
//{
//	-1.0 / 3.0,
//	1.0 / 3.0,
//	-7.0 / 9.0,
//	-1.0 / 9.0,
//	5.0 / 9.0,
//	-5.0 / 9.0,
//	1.0 / 9.0,
//	7.0 / 9.0
//};
