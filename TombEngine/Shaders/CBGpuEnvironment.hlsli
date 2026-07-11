#ifndef CBGPUENVIRONMENT
#define CBGPUENVIRONMENT

#define GPU_ENVIRONMENT_STARFIELD 0
#define GPU_ENVIRONMENT_UNDERWATER_DUST 1
#define GPU_ENVIRONMENT_SNOW 2
#define GPU_ENVIRONMENT_RAIN 3

#define GPU_ENVIRONMENT_TEXTURE_BUCKET 0
#define GPU_ENVIRONMENT_TEXTURE_ATLAS 1
#define GPU_ENVIRONMENT_TEXTURE_ARRAY 2

cbuffer CBGpuEnvironment : register(b13)
{
	float4 EnvironmentUV[2];
	int EnvironmentMode;
	int EnvironmentClusterStride;
	float EnvironmentClusterSpread;
	int EnvironmentParticleOffset;
	int EnvironmentTextureMode;
	int EnvironmentPadding0;
	int EnvironmentPadding1;
	int EnvironmentPadding2;
};

#endif
