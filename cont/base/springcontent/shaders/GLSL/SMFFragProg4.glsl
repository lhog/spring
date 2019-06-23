#version 410 core

const float SMF_SHALLOW_WATER_DEPTH     = 10.0;
const float SMF_SHALLOW_WATER_DEPTH_INV = 1.0 / SMF_SHALLOW_WATER_DEPTH;
const float SMF_DETAILTEX_RES           = 0.02;

const float PI = 3.1415926535897932384626433832795;
const float DEGREES_TO_RADIANS = PI / 180.0;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

uniform sampler2DArray diffuseTex;
uniform sampler2D      normalsTex;
uniform sampler2D      detailTex;

uniform vec2 normalTexGen;   // either 1.0/mapSize (when NPOT are supported) or 1.0/mapSizePO2
uniform vec2 specularTexGen; // 1.0/mapSize

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;
uniform vec3 groundSpecularColor;
uniform float groundSpecularExponent;
uniform float groundShadowDensity;

uniform float gammaExponent;

uniform vec4 alphaTestCtrl;

uniform vec2 mapHeights; // min & max height on the map

uniform vec4 lightDir;
uniform vec3 cameraPos;
uniform vec4 fogColor;

uniform mat4 viewMat;

uniform vec4 fwdDynLights[MAX_LIGHT_UNIFORM_VECS];

uniform ivec4 texSquare;



#ifdef HAVE_INFOTEX
	uniform sampler2D infoTex;
	uniform float infoTexIntensityMul;
	uniform vec2 infoTexGen;     // 1.0/(pwr2map{x,z} * SQUARE_SIZE)
#endif

#ifdef SMF_SPECULAR_LIGHTING
	uniform sampler2D specularTex;
#endif

#if (!defined(DEFERRED_MODE) && defined(HAVE_SHADOWS))
	uniform sampler2DShadow shadowTex;
	uniform mat4 shadowMat;
	uniform vec4 shadowParams; //unused, will be removed by compiler

	#define SHADOW_HARD 0
	#define SHADOW_SOFT 1
	#define SHADOW_SOFTER 2
	#define SHADOW_SOFTEST 3

	#if (SHADOW_SOFTNESS == SHADOW_HARD)
		#define SHADOW_SAMPLES 1
	#endif

	#if (SHADOW_SOFTNESS == SHADOW_SOFT)
		#define SHADOW_SAMPLES 3 // number of shadowmap samples per fragment
		#define SHADOW_RANDOMNESS 1.0 // 0.0 - blocky look, 1.0 - random points look
		#define SHADOW_SAMPLING_DISTANCE 2.0 // how far shadow samples go (in shadowmap texels) as if it was applied to 8192x8192 sized shadow map
	#endif

	#if (SHADOW_SOFTNESS == SHADOW_SOFTER)
		#define SHADOW_SAMPLES 6 // number of shadowmap samples per fragment
		#define SHADOW_RANDOMNESS 0.8 // 0.0 - blocky look, 1.0 - random points look
		#define SHADOW_SAMPLING_DISTANCE 3.0 // how far shadow samples go (in shadowmap texels) as if it was applied to 8192x8192 sized shadow map
	#endif

	#if (SHADOW_SOFTNESS == SHADOW_SOFTEST)
		#define SHADOW_SAMPLES 8 // number of shadowmap samples per fragment
		#define SHADOW_RANDOMNESS 0.8 // 0.0 - blocky look, 1.0 - random points look
		#define SHADOW_SAMPLING_DISTANCE 4.0 // how far shadow samples go (in shadowmap texels) as if it was applied to 8192x8192 sized shadow map
	#endif
	#if (SHADOW_SOFTNESS > SHADOW_HARD)
		uniform vec2 lightProjScale;
	#else
		const vec2 lightProjScale = vec2(1.0); // unused
	#endif
#endif

#ifdef SMF_WATER_ABSORPTION
	uniform vec3 waterMinColor;
	uniform vec3 waterBaseColor;
	uniform vec3 waterAbsorbColor;
#endif

#if defined(SMF_DETAIL_TEXTURE_SPLATTING) && !defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING)
	uniform sampler2D splatDetailTex;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif

#ifdef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
	uniform sampler2D splatDetailNormalTex1;
	uniform sampler2D splatDetailNormalTex2;
	uniform sampler2D splatDetailNormalTex3;
	uniform sampler2D splatDetailNormalTex4;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif
#ifdef SMF_SKY_REFLECTIONS
	uniform samplerCube skyReflectTex;
	uniform sampler2D skyReflectModTex;
#endif

#ifdef SMF_BLEND_NORMALS
	uniform sampler2D blendNormalsTex;
#endif

#ifdef SMF_LIGHT_EMISSION
	uniform sampler2D lightEmissionTex;
#endif

#ifdef SMF_PARALLAX_MAPPING
	uniform sampler2D parallaxHeightTex;
#endif



in vec3 halfDir;
in vec4 vertexPos;
in vec2 diffuseTexCoords;
#if (!defined(DEFERRED_MODE) && defined(HAVE_SHADOWS))
	in vec4 vertexShadowPos;
#endif

in float fogFactor;

#ifdef DEFERRED_MODE
layout(location = 0) out vec4 fragData[SMF_FRAGDATA_COUNT];
#else
layout(location = 0) out vec4 fragColor;
#endif



#ifdef SMF_PARALLAX_MAPPING
vec2 GetParallaxUVOffset(vec2 uv, vec3 dir) {
	vec4 texel = texture(parallaxHeightTex, uv);

	// RG: height in [ 0.0, 1.0] (256^2 strata)
	//  B: scale  in [ 0.0, 1.0] (256   strata), eg.  0.04 (~10.0/256.0)
	//  A: bias   in [-0.5, 0.5] (256   strata), eg. -0.02 (~75.0/256.0)
	//
	const float RMUL = 255.0 * 256.0;
	const float GMUL = 256.0;
	const float HDIV = 65536.0;

	float heightValue  = dot(texel.rg, vec2(RMUL, GMUL)) / HDIV;
	float heightScale  = texel.b;
	float heightBias   = texel.a - 0.5;
	float heightOffset = heightValue * heightScale + heightBias;

	return ((dir.xy / dir.z) * heightOffset);
}
#endif

vec3 GetFragmentNormal(vec2 uv) {
	vec3 normal;
	#ifdef SSMF_UNCOMPRESSED_NORMALS
	normal = normalize(texture(normalsTex, uv).xyz);
	#else
	normal.xz = texture(normalsTex, uv).rg;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	#endif
	return normal;
}


#ifndef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
vec4 GetDetailTextureColor(vec2 uv) {
	#ifndef SMF_DETAIL_TEXTURE_SPLATTING
		vec2 detailTexCoord = vertexPos.xz * vec2(SMF_DETAILTEX_RES);
		vec4 detailCol = NORM2SNORM(texture(detailTex, detailTexCoord));
	#else
		vec4 splatTexCoord0 = vertexPos.xzxz * splatTexScales.rrgg;
		vec4 splatTexCoord1 = vertexPos.xzxz * splatTexScales.bbaa;
		vec4 splatDetails;
			splatDetails.r = texture(splatDetailTex, splatTexCoord0.st).r;
			splatDetails.g = texture(splatDetailTex, splatTexCoord0.pq).g;
			splatDetails.b = texture(splatDetailTex, splatTexCoord1.st).b;
			splatDetails.a = texture(splatDetailTex, splatTexCoord1.pq).a;
			splatDetails   = NORM2SNORM(splatDetails);
		vec4 splatDist = texture(splatDistrTex, uv) * splatTexMults;
		vec4 detailCol = vec4(dot(splatDetails, splatDist));
	#endif
	return detailCol;
}

#else

vec4 GetSplatDetailTextureNormal(vec2 uv, out vec2 splatDetailStrength) {
	vec4 splatTexCoord0 = vertexPos.xzxz * splatTexScales.rrgg;
	vec4 splatTexCoord1 = vertexPos.xzxz * splatTexScales.bbaa;
	vec4 splatDist = texture(splatDistrTex, uv) * splatTexMults;

	// dot with 1's to sum up the splat distribution weights
	splatDetailStrength.x = min(1.0, dot(splatDist, vec4(1.0)));

	vec4 splatDetailNormal;
		splatDetailNormal  = NORM2SNORM(texture(splatDetailNormalTex1, splatTexCoord0.st)) * splatDist.r;
		splatDetailNormal += NORM2SNORM(texture(splatDetailNormalTex2, splatTexCoord0.pq)) * splatDist.g;
		splatDetailNormal += NORM2SNORM(texture(splatDetailNormalTex3, splatTexCoord1.st)) * splatDist.b;
		splatDetailNormal += NORM2SNORM(texture(splatDetailNormalTex4, splatTexCoord1.pq)) * splatDist.a;

	// note: y=0.01 (pointing up) in case all splat-cofacs are zero
	splatDetailNormal.y = max(splatDetailNormal.y, 0.01);

	#ifdef SMF_DETAIL_NORMAL_DIFFUSE_ALPHA
		splatDetailStrength.y = clamp(splatDetailNormal.a, -1.0, 1.0);
	#endif

	// note: .xyz is intentionally not normalized here
	// (the normal will be rotated to worldspace first)
	return splatDetailNormal;
}
#endif


vec4 GetShadeInt(float groundLightInt, float groundShadowCoeff, float groundDiffuseAlpha) {
	vec4 groundShadeInt = vec4(0.0, 0.0, 0.0, 1.0);

	groundShadeInt.rgb = groundAmbientColor + groundDiffuseColor * (groundLightInt * groundShadowCoeff);
	groundShadeInt.rgb *= vec3(SMF_INTENSITY_MULT);

	#ifdef SMF_VOID_WATER
	// cut out all underwater fragments indiscriminately
	groundShadeInt.a = float(vertexPos.y >= 0.0);
	#endif

	#ifdef SMF_VOID_GROUND
	// assume the map(per)'s diffuse texture provides sensible alphas
	// note that voidground overrides voidwater if *both* are enabled
	// (limiting it to just above-water fragments would be arbitrary)
	groundShadeInt.a = groundDiffuseAlpha;
	#endif

	#ifdef SMF_WATER_ABSORPTION
	// use groundShadeInt alpha value; allows voidground maps to create
	// holes in the seabed (SMF_WATER_ABSORPTION == 1 implies voidwater
	// is not enabled but says nothing about the voidground state)
	vec4 rawWaterShadeInt = vec4(waterBaseColor.rgb, groundShadeInt.a);
	vec4 modWaterShadeInt = rawWaterShadeInt;

	{ //if (mapHeights.x <= 0.0) {
		float waterShadeAlpha  = abs(vertexPos.y) * SMF_SHALLOW_WATER_DEPTH_INV;
		float waterShadeDecay  = 0.2 + (waterShadeAlpha * 0.1);
		float vertexStepHeight = min(1023.0, -vertexPos.y);
		float waterLightInt    = min(groundLightInt * 2.0 + 0.4, 1.0);

		// vertex below shallow water depth --> alpha=1
		// vertex above shallow water depth --> alpha=waterShadeAlpha
		waterShadeAlpha = min(1.0, waterShadeAlpha + float(vertexPos.y <= -SMF_SHALLOW_WATER_DEPTH));

		modWaterShadeInt.rgb -= (waterAbsorbColor.rgb * vertexStepHeight);
		modWaterShadeInt.rgb  = max(waterMinColor.rgb, modWaterShadeInt.rgb);
		modWaterShadeInt.rgb *= vec3(SMF_INTENSITY_MULT * waterLightInt);

		// make shadowed areas darker over deeper water
		modWaterShadeInt.rgb *= (1.0 - waterShadeDecay * (1.0 - groundShadowCoeff));

		// if depth is greater than _SHALLOW_ depth, select waterShadeInt
		// otherwise interpolate between groundShadeInt and waterShadeInt
		// (both are already cosine-weighted)
		modWaterShadeInt.rgb = mix(groundShadeInt.rgb, modWaterShadeInt.rgb, waterShadeAlpha);
	}

	modWaterShadeInt = mix(rawWaterShadeInt, modWaterShadeInt, float(mapHeights.x <= 0.0));
	groundShadeInt = mix(groundShadeInt, modWaterShadeInt, float(vertexPos.y < 0.0));
	#endif

	return groundShadeInt;
}


vec3 DynamicLighting(vec3 wsNormal, vec3 camDir, vec3 diffuseColor, vec4 specularColor) {
	vec3 light = vec3(0.0);

	#ifndef SMF_SPECULAR_LIGHTING
	// non-zero default specularity on non-SSMF maps
	specularColor.rgb = vec3(0.5, 0.5, 0.5);
	#endif

	#if (NUM_DYNAMIC_MAP_LIGHTS > 0)
	for (int i = 0; i < NUM_DYNAMIC_MAP_LIGHTS; i++) {
		int j = i * 6;

		vec4 wsLightPos = fwdDynLights[j + 0];
		vec4 wsLightDir = fwdDynLights[j + 1];

		vec4 lightDiffColor = fwdDynLights[j + 2];
		vec4 lightSpecColor = fwdDynLights[j + 3];
		vec4 lightAmbiColor = fwdDynLights[j + 4];

		vec3 wsLightVec = normalize(wsLightPos.xyz - vertexPos.xyz);
		vec3 wsHalfVec = normalize(camDir + wsLightVec);

		float lightAngle    = fwdDynLights[j + 5].x; // fov
		float lightRadius   = fwdDynLights[j + 5].y; // or const. atten.
		float lightDistance = dot(wsLightVec, wsLightPos.xyz - vertexPos.xyz);

		// clamp lightCosAngleSpec from 0.001 because pow(0, exp) produces undefined results
		float lightCosAngleDiff = clamp(dot(wsNormal, wsLightVec), 0.0, 1.0);
		float lightCosAngleSpec = clamp(dot(wsNormal, wsHalfVec), 0.001, 1.0);

		#ifdef OGL_SPEC_ATTENUATION
		// infinite falloff
		float cLightAtten = fwdDynLights[j + 5].y;
		float lLightAtten = fwdDynLights[j + 5].z;
		float qLightAtten = fwdDynLights[j + 5].w;
		float  lightAtten = cLightAtten + lLightAtten * lightDistance + qLightAtten * lightDistance * lightDistance;
		float  lightConst = 1.0;

		lightAtten = 1.0 / max(lightAtten, 1.0);
		#else
		float lightConst = float(lightDistance <= lightRadius);
		float lightAtten = 1.0 - min(1.0, ((lightDistance * lightDistance) / (lightRadius * lightRadius)));
		#endif

		#ifdef SMF_SPECULAR_LIGHTING
		float lightSpecularPow = max(0.0, pow(lightCosAngleSpec, specularColor.a));
		#else
		float lightSpecularPow = 0.0;
		#endif

		float vectorCosAngle = dot(-wsLightVec, wsLightDir.xyz);
		float cutoffCosAngle = cos(lightAngle * DEGREES_TO_RADIANS);

		lightConst *= float(vectorCosAngle >= cutoffCosAngle);

		light += (lightConst *                                   lightAmbiColor.rgb                     );
		light += (lightConst * lightAtten * ( diffuseColor.rgb * lightDiffColor.rgb * lightCosAngleDiff));
		light += (lightConst * lightAtten * (specularColor.rgb * lightSpecColor.rgb * lightSpecularPow ));
	}
	#endif

	return light;
}

#if (!defined(DEFERRED_MODE) && defined(HAVE_SHADOWS))
float hash12L(vec2 p) {
	const float HASHSCALE1 = 0.1031;
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}

// http://blog.marmakoide.org/?p=1
const float goldenAngle = 2.3999632297286533222315555066336;
vec2 SpiralSNorm(int i, int N) {
	float theta = float(i) * goldenAngle;
	float r = sqrt(float(i)) / sqrt(float(N));
	return vec2(r * cos(theta), r * sin(theta));
}

// Derivatives of light-space depth with respect to texture2D coordinates
vec2 DepthGradient(vec2 uv, float z) {
	vec2 dZduv = vec2(0.0, 0.0);

	vec3 dUVZdx = dFdx(vec3(uv,z));
	vec3 dUVZdy = dFdy(vec3(uv,z));

	dZduv.x = dUVZdy.y * dUVZdx.z;
	dZduv.x -= dUVZdx.y * dUVZdy.z;

	dZduv.y = dUVZdx.x * dUVZdy.z;
	dZduv.y -= dUVZdy.x * dUVZdx.z;

	float det = (dUVZdx.x * dUVZdy.y) - (dUVZdx.y * dUVZdy.x);
	dZduv /= det;

	return dZduv;
}

float BiasedZ(float z0, vec2 dZduv, vec2 offset) {
	return z0 + dot(dZduv, offset);
}

float GetShadowPCF(float NdotL, vec2 shadowScaleFactor) {
	float shadow = 0.0;

	vec3 shadowCoordN = vertexShadowPos.xyz / vertexShadowPos.w;
	#if defined(SHADOW_SAMPLES) && (SHADOW_SAMPLES > 1)
		vec2 dZduv = DepthGradient(shadowCoordN.xy, shadowCoordN.z);

		float rndRotAngle = NORM2SNORM(hash12L(gl_FragCoord.xy)) * PI / 2.0 * SHADOW_RANDOMNESS;

		vec2 vSinCos = vec2(sin(rndRotAngle), cos(rndRotAngle));
		mat2 rotMat = mat2(vSinCos.y, -vSinCos.x, vSinCos.x, vSinCos.y);

		vec2 filterSize = vec2(SHADOW_SAMPLING_DISTANCE / 8192.0) * shadowScaleFactor;

		for (int i = 0; i < SHADOW_SAMPLES; ++i) {
			// SpiralSNorm return low discrepancy sampling vec2
			vec2 offset = (rotMat * SpiralSNorm( i, SHADOW_SAMPLES )) * filterSize;

			vec3 shadowSamplingCoord = vec3(shadowCoordN.xy, 0.0) + vec3(offset, BiasedZ(shadowCoordN.z, dZduv, offset));
			shadow += texture( shadowTex, shadowSamplingCoord );
		}

		shadow /= float(SHADOW_SAMPLES);
		shadow *= 1.0 - smoothstep(shadow, 1.0,  0.2);
	#else
		const float cb = 0.00005;
		float bias = cb * tan(acos(NdotL));
		bias = clamp(bias, 0.0, 5.0 * cb);

		vec3 shadowSamplingCoord = shadowCoordN;
		shadowSamplingCoord.z -= bias;
		shadow = texture( shadowTex, shadowSamplingCoord );
	#endif

	return mix(1.0, shadow, groundShadowDensity);
}
#endif

void main() {
	vec2 diffTexCoords = diffuseTexCoords;
	vec2 specTexCoords = vertexPos.xz * specularTexGen;
	vec2 normTexCoords = vertexPos.xz * normalTexGen;

	vec3 cameraDir = vertexPos.xyz - cameraPos;
	vec3 normalVec = GetFragmentNormal(normTexCoords);

	#if (defined(SMF_BLEND_NORMALS) || defined(SMF_PARALLAX_MAPPING) || defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING))
		// detail-normals are (assumed to be) defined within STN space
		// (for a regular vertex normal equal to <0, 1, 0>, the S- and
		// T-tangents are aligned with Spring's +x and +z (!) axes)
		vec3 tTangent = normalize(cross(normalVec, vec3(-1.0, 0.0, 0.0)));
		vec3 sTangent = cross(normalVec, tTangent);
		mat3 stnMatrix = mat3(sTangent, tTangent, normalVec);
	#endif

	#ifdef SMF_PARALLAX_MAPPING
	{
		// use specular-texture coordinates to index parallaxHeightTex
		// (ie. specularTex and parallaxHeightTex must have equal size)
		// cameraDir does not need to be normalized, x/z and y/z ratios
		// do not change
		vec2 uvOffset = GetParallaxUVOffset(specTexCoords, transpose(stnMatrix) * cameraDir);

		// scale the parallax offset since it is in spectex-space
		diffTexCoords += (uvOffset / (SMF_TEXSQUARE_SIZE * specularTexGen));
		normTexCoords += (uvOffset * (normalTexGen / specularTexGen));
		specTexCoords += (uvOffset);

		normalVec = GetFragmentNormal(normTexCoords);
	}
	#endif

	#ifdef SMF_BLEND_NORMALS
	{
		vec4 dtSample = texture(blendNormalsTex, normTexCoords);
		vec3 dtNormal = NORM2SNORM(dtSample.xyz);

		// convert dtNormal from TS to WS before mixing
		normalVec = normalize(mix(normalVec, stnMatrix * dtNormal, dtSample.a));
	}
	#endif


	vec4 detailColor;

	#ifndef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
	{
		detailColor = GetDetailTextureColor(specTexCoords);
	}
	#else
	{
		// x-component modulates mixing of normals
		// y-component contains the detail color (splatDetailNormal.a if SMF_DETAIL_NORMAL_DIFFUSE_ALPHA)
		vec2 splatDetailStrength = vec2(0.0, 0.0);

		// note: splatDetailStrength is an OUT-param
		vec4 splatDetailNormal = GetSplatDetailTextureNormal(specTexCoords, splatDetailStrength);

		detailColor = vec4(splatDetailStrength.y);

		// convert the splat detail normal to worldspace,
		// then mix it with the regular one (note: needs
		// another normalization?)
		normalVec = mix(normalVec, normalize(stnMatrix * splatDetailNormal.xyz), splatDetailStrength.x);
	}
	#endif

	float NdotL = dot(lightDir.xyz, normalVec);
	#ifndef DEFERRED_MODE
	float cosAngleDiffuse = clamp(NdotL, 0.0, 1.0);
	float cosAngleSpecular = clamp(dot(normalize(halfDir), normalVec), 0.001, 1.0);
	#endif

	vec4  diffuseColor = textureLod(diffuseTex, vec3(diffTexCoords, texSquare.z), texSquare.w);
	vec4 specularColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 emissionColor = vec4(0.0, 0.0, 0.0, 0.0);

	#if (!defined(DEFERRED_MODE) && defined(SMF_SKY_REFLECTIONS))
	{
		// cameraDir does not need to be normalized for reflect()
		// note: assumes viewMat is always orthonormal, otherwise
		// requires the inverse-transpose
		vec4 reflectDir = viewMat * vec4(reflect(cameraDir, normalVec), 0.0);

		vec3 reflectColor = texture(skyReflectTex, reflectDir.xyz).rgb;
		vec3 reflectCoeff = texture(skyReflectModTex, specTexCoords).rgb;

		diffuseColor.rgb = mix(diffuseColor.rgb, reflectColor, reflectCoeff);
	}
	#endif
	#if (!defined(DEFERRED_MODE) && defined(HAVE_INFOTEX))
	{
		// increase contrast and brightness for the overlays
		// TODO: make the multiplier configurable by users?
		vec2 infoTexCoords = vertexPos.xz * infoTexGen;

		diffuseColor.rgb += (texture(infoTex, infoTexCoords).rgb * infoTexIntensityMul);
		diffuseColor.rgb -= (vec3(0.5, 0.5, 0.5) * float(infoTexIntensityMul == 1.0));
	}
	#endif

	float shadowCoeff = 1.0;
	{
		float nShadowMix = smoothstep(0.0, 0.35, NdotL); //use unclamped NdotL
		float nShadowCoeff = mix(1.0, nShadowMix, groundShadowDensity); //NdotL based "shadow"

		#if (!defined(DEFERRED_MODE) && defined(HAVE_SHADOWS))
			float gShadowCoeff = GetShadowPCF(cosAngleDiffuse, lightProjScale); //use clamped NdotL
		#else
			float gShadowCoeff = 1.0;
		#endif

		shadowCoeff = min(nShadowCoeff, gShadowCoeff);
	}

	#ifndef DEFERRED_MODE
	{
		// diffuse shading intensity
		vec4 shadeInt = GetShadeInt(cosAngleDiffuse, shadowCoeff, diffuseColor.a);

		fragColor.rgb = (diffuseColor.rgb + detailColor.rgb) * shadeInt.rgb;
		fragColor.a = shadeInt.a;
	}
	#endif
	#ifndef DEFERRED_MODE
	{
		float alphaTestGreater = float(fragColor.a > alphaTestCtrl.x) * alphaTestCtrl.y;
		float alphaTestSmaller = float(fragColor.a < alphaTestCtrl.x) * alphaTestCtrl.z;

		if ((alphaTestGreater + alphaTestSmaller + alphaTestCtrl.w) == 0.0)
			discard;
	}
	#endif

	#ifdef SMF_LIGHT_EMISSION
	{
		// apply self-illumination aka. glow, not masked by shadows
		emissionColor = texture(lightEmissionTex, specTexCoords);

		#ifndef DEFERRED_MODE
		fragColor.rgb = fragColor.rgb * (1.0 - emissionColor.a) + emissionColor.rgb;
		#endif
	}
	#endif

	#ifdef SMF_SPECULAR_LIGHTING
	specularColor = texture(specularTex, specTexCoords);
	#else
	specularColor = vec4(groundSpecularColor, 1.0);
	#endif

	#ifndef DEFERRED_MODE
		// sun specular lighting contribution
		#ifdef SMF_SPECULAR_LIGHTING
		float specularExp = specularColor.a * 16.0;
		#else
		float specularExp = groundSpecularExponent;
		#endif
		float specularPow = max(0.0, pow(cosAngleSpecular, specularExp));

		fragColor.rgb += (specularColor.rgb * specularPow * shadowCoeff);
		fragColor.rgb += DynamicLighting(normalVec, cameraDir * -1.0, diffuseColor.rgb, vec4(specularColor.rgb, specularExp));
	#endif



	#ifdef DEFERRED_MODE
	fragData[GBUFFER_NORMTEX_IDX] = vec4(SNORM2NORM(normalVec), 1.0);
	fragData[GBUFFER_DIFFTEX_IDX] = diffuseColor + detailColor;
	fragData[GBUFFER_SPECTEX_IDX] = specularColor;
	fragData[GBUFFER_EMITTEX_IDX] = emissionColor;
	fragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	#else
	fragColor.rgb = mix(fogColor.rgb, fragColor.rgb, fogFactor);
	fragColor.rgb = pow(fragColor.rgb, vec3(gammaExponent));
	#endif
}

