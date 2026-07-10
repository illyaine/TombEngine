#ifndef CBSTARFIELD
#define CBSTARFIELD

#define GPU_ENVIRONMENT_STARFIELD 0
#define GPU_ENVIRONMENT_UNDERWATER_DUST 1
#define GPU_ENVIRONMENT_SNOW 2
#define GPU_ENVIRONMENT_RAIN 3

cbuffer CBStarfield : register(b13)
{
	float4 EnvironmentUV[2];
	int EnvironmentMode;
	int EnvironmentClusterStride;
	float EnvironmentClusterSpread;
	float EnvironmentPadding;
};

#endif
