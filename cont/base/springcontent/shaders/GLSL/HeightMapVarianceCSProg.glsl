#version 430

//layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(local_size_x = LSX, local_size_y = LSY, local_size_z = LSZ) in;

uniform sampler2D hmTex;
layout(binding = 0, r32f) uniform readonly image2D inImg;
layout(binding = 1, r32f) uniform writeonly image2D outImg;

uniform int mipLvl;

//central element difference is:  -0.012695000000000118
vec4 LoG9x9(ivec2 tc) {
     vec4 R = vec4(0.0);
    R += 0.000169 * imageLoad(inImg, tc + ivec2(-4, -4));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(-4, -3));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(-4, -2));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(-4, -1));
    R += 0.004310 * imageLoad(inImg, tc + ivec2(-4, 0));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(-4, 1));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(-4, 2));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(-4, 3));
    R += 0.000169 * imageLoad(inImg, tc + ivec2(-4, 4));

    R += 0.000757 * imageLoad(inImg, tc + ivec2(-3, -4));
    R += 0.003016 * imageLoad(inImg, tc + ivec2(-3, -3));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(-3, -2));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(-3, -1));
    R += 0.010810 * imageLoad(inImg, tc + ivec2(-3, 0));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(-3, 1));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(-3, 2));
    R += 0.003016 * imageLoad(inImg, tc + ivec2(-3, 3));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(-3, 4));

    R += 0.002068 * imageLoad(inImg, tc + ivec2(-2, -4));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(-2, -3));
    R += 0.011205 * imageLoad(inImg, tc + ivec2(-2, -2));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(-2, -1));
    R += 0.000610 * imageLoad(inImg, tc + ivec2(-2, 0));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(-2, 1));
    R += 0.011205 * imageLoad(inImg, tc + ivec2(-2, 2));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(-2, 3));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(-2, 4));

    R += 0.003616 * imageLoad(inImg, tc + ivec2(-1, -4));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(-1, -3));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(-1, -2));
    R += -0.024365 * imageLoad(inImg, tc + ivec2(-1, -1));
    R += -0.047824 * imageLoad(inImg, tc + ivec2(-1, 0));
    R += -0.024365 * imageLoad(inImg, tc + ivec2(-1, 1));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(-1, 2));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(-1, 3));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(-1, 4));

    R += 0.004310 * imageLoad(inImg, tc + ivec2(0, -4));
    R += 0.010810 * imageLoad(inImg, tc + ivec2(0, -3));
    R += 0.000610 * imageLoad(inImg, tc + ivec2(0, -2));
    R += -0.047824 * imageLoad(inImg, tc + ivec2(0, -1));
    R += -0.070164 * imageLoad(inImg, tc + ivec2(0, 0));
    R += -0.047824 * imageLoad(inImg, tc + ivec2(0, 1));
    R += 0.000610 * imageLoad(inImg, tc + ivec2(0, 2));
    R += 0.010810 * imageLoad(inImg, tc + ivec2(0, 3));
    R += 0.004310 * imageLoad(inImg, tc + ivec2(0, 4));

    R += 0.003616 * imageLoad(inImg, tc + ivec2(1, -4));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(1, -3));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(1, -2));
    R += -0.024365 * imageLoad(inImg, tc + ivec2(1, -1));
    R += -0.047824 * imageLoad(inImg, tc + ivec2(1, 0));
    R += -0.024365 * imageLoad(inImg, tc + ivec2(1, 1));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(1, 2));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(1, 3));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(1, 4));

    R += 0.002068 * imageLoad(inImg, tc + ivec2(2, -4));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(2, -3));
    R += 0.011205 * imageLoad(inImg, tc + ivec2(2, -2));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(2, -1));
    R += 0.000610 * imageLoad(inImg, tc + ivec2(2, 0));
    R += 0.006376 * imageLoad(inImg, tc + ivec2(2, 1));
    R += 0.011205 * imageLoad(inImg, tc + ivec2(2, 2));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(2, 3));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(2, 4));

    R += 0.000757 * imageLoad(inImg, tc + ivec2(3, -4));
    R += 0.003016 * imageLoad(inImg, tc + ivec2(3, -3));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(3, -2));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(3, -1));
    R += 0.010810 * imageLoad(inImg, tc + ivec2(3, 0));
    R += 0.010024 * imageLoad(inImg, tc + ivec2(3, 1));
    R += 0.006964 * imageLoad(inImg, tc + ivec2(3, 2));
    R += 0.003016 * imageLoad(inImg, tc + ivec2(3, 3));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(3, 4));

    R += 0.000169 * imageLoad(inImg, tc + ivec2(4, -4));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(4, -3));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(4, -2));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(4, -1));
    R += 0.004310 * imageLoad(inImg, tc + ivec2(4, 0));
    R += 0.003616 * imageLoad(inImg, tc + ivec2(4, 1));
    R += 0.002068 * imageLoad(inImg, tc + ivec2(4, 2));
    R += 0.000757 * imageLoad(inImg, tc + ivec2(4, 3));
    R += 0.000169 * imageLoad(inImg, tc + ivec2(4, 4));

    return R;
}

vec4 LaplacianD0(ivec2 tc) {
	vec4 R = vec4(0.0);
	R += -4.0 * imageLoad(inImg, tc + ivec2(0, 0));

	R += imageLoad(inImg, tc + ivec2(-1, 0));
	R += imageLoad(inImg, tc + ivec2( 1, 0));
	R += imageLoad(inImg, tc + ivec2(0, -1));
	R += imageLoad(inImg, tc + ivec2(0,  1));

	return R;
}

vec4 LaplacianD1(ivec2 tc) {
	vec4 R = vec4(0.0);
	R += -8.0 * imageLoad(inImg, tc + ivec2(0, 0));

	R += imageLoad(inImg, tc + ivec2(-1, -1));
	R += imageLoad(inImg, tc + ivec2( 0, -1));
	R += imageLoad(inImg, tc + ivec2( 1, -1));
	R += imageLoad(inImg, tc + ivec2(-1,  0));
	R += imageLoad(inImg, tc + ivec2( 1,  0));
	R += imageLoad(inImg, tc + ivec2(-1,  1));
	R += imageLoad(inImg, tc + ivec2( 0,  1));
	R += imageLoad(inImg, tc + ivec2( 1,  1));

	return R;
}

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

void CopyCenteredPass() {
	vec2 ts = vec2(imageSize(outImg));
	ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);

	if (any(greaterThanEqual(texCoord, ivec2(ts))))
		return;

	vec2 mapUV = vec2(texCoord + vec2(1.0)) / ts; //no idea why but it works
#if 1
	//rescale to not sample edge pixels
	mapUV = NORM2SNORM(mapUV);
	mapUV *= (ts - vec2(1.0)) / ts;
	mapUV = SNORM2NORM(mapUV);
#endif
	
	float height = textureLod(hmTex, mapUV, 0.0).x;

	imageStore(outImg, texCoord, vec4(height));
}



void LoGPass() {
	ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
	vec4 logResult = 1.0 * abs(LoG9x9(2 * texCoord));
	//vec4 logResult = 1.0 * abs(LaplacianD1(2 * texCoord));
	//vec4 logResult = abs(LaplacianD1(2 * texCoord));
	imageStore(outImg, texCoord, vec4(logResult.xxxx));
	//imageStore(outImg, texCoord, vec4( logResult.xxxx));
}

void HiZPass() {
	ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
	vec4 hiZResult = vec4(0.0);

	hiZResult = max(hiZResult, imageLoad(inImg, 2 * texCoord + ivec2(0, 0)));
	hiZResult = max(hiZResult, imageLoad(inImg, 2 * texCoord + ivec2(0, 1)));
	hiZResult = max(hiZResult, imageLoad(inImg, 2 * texCoord + ivec2(1, 0)));
	hiZResult = max(hiZResult, imageLoad(inImg, 2 * texCoord + ivec2(1, 1)));
	imageStore(outImg, texCoord, hiZResult);
}


void main() {
	if (mipLvl == -1)
		CopyCenteredPass();
	else if (mipLvl == 0)
		LoGPass();
	else
		HiZPass();
}