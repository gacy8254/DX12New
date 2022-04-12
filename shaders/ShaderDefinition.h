
//�������shader��һЩ���ú궨��

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

//ʹ��reverseZ
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

//ʹ����õ�˥����ʽ
#define UNREAL_LIGHT_ATTENUATION 1

//ʹ��TAA
#define USE_TAA 0

#define TAA_SAMPLE_COUNT 8
#define TAA_JITTER_DISTANCE 1.0;

//----------------------------------------------------------------------------------------------------------
// Light
//----------------------------------------------------------------------------------------------------------

#define MAX_GRID_POINT_LIGHT_NUM 80
#define MAX_GRID_SPOTLIGHT_NUM 20

//----------------------------------------------------------------------------------------------------------
// TBDR
//----------------------------------------------------------------------------------------------------------

#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

#define TILE_THREAD_NUM_X 8
#define TILE_THREAD_NUM_Y 8

#define COMPUTE_SHADER_TILE_GROUP_SIZE (TILE_THREAD_NUM_X * TILE_THREAD_NUM_Y)

//----------------------------------------------------------------------------------------------------------
// CBDR
//----------------------------------------------------------------------------------------------------------

#define CLUSTER_SIZE_X 128	
#define CLUSTER_SIZE_Y 128
#define CLUSTER_NUM_Z 64

#define CLUSTER_THREAD_NUM_X 8
#define CLUSTER_THREAD_NUM_Y 8

#define COMPUTE_SHADER_CLUSTER_GROUP_SIZE (CLUSTER_THREAD_NUM_X * CLUSTER_THREAD_NUM_Y)

//----------------------------------------------------------------------------------------------------------
// Shadow
//----------------------------------------------------------------------------------------------------------
#define CAST_SHADOW 1
