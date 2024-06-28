#version 450

layout(push_constant) uniform PushConstants
{
	mat4 model;
	vec3 sunDir;
}pc;

layout(location = 0) in vec3 rayDir;

layout(location = 0) out vec4 outColor;


float earthRadius = 6371000.0;
float atmosphereRadius = 6420000.0;
float Hr = 7994.0; // Thickness of the atmosphere if density was uniform (Hr) 
float Hm = 1200.0; // Same as above but for Mie scattering (Hm)
vec3 betaR = vec3(5.8e-6, 13.5e-6, 33.1e-6); // Rayleigh scattering coefficient
vec3 betaM = vec3(21e-6); // Mie scattering coefficient

float sunIntensity = 20.0;
float mieG = 0.76;

float numSamples = 16.0;
float numSamplesLight = 8.0;

bool raySphereIntersect(vec3 origin, vec3 dir, float radius, out float t0, out float t1)
{
	vec3 L = origin;
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, L);
	float c = dot(L, L) - radius * radius;
	float discriminant = b * b - 4.0 * a * c;
	if (discriminant < 0.0)
		return false;
	discriminant = sqrt(discriminant);
	t0 = (-b - discriminant) / (2.0 * a);
	t1 = (-b + discriminant) / (2.0 * a);
	if (t0 > t1)
	{
		float temp = t0;
		t0 = t1;
		t1 = temp;
	}
	return true;
}

float rayleighPhase(float cosTheta)
{
	return 3.0 / (16.0 * 3.14159265359) * (1.0 + cosTheta * cosTheta);
}

float miePhase(float cosTheta)
{
	return 3.0 / (8.0 * 3.14159265359) * ((1.0 - mieG * mieG) * (1.0 + cosTheta * cosTheta)) / pow(1.0 + mieG * mieG - 2.0 * mieG * cosTheta, 1.5);
}

vec3 computeSkyColor(vec3 origin, vec3 dir, float tmin, float tmax)
{
	float t0, t1;
	if (!raySphereIntersect(origin, dir, atmosphereRadius, t0, t1))
		return vec3(1.0, 0.0, 0.0);
	if (t0 > tmin && t0 > 0) tmin = t0;
    if (t1 < tmax) tmax = t1;
	float segmentLength = (tmax - tmin) / numSamples;
    float tCurrent = tmin;
    vec3 sumR = vec3(0), sumM = vec3(0);  // mie and rayleigh contribution
    float opticalDepthR = 0, opticalDepthM = 0;
    float mu = dot(dir, pc.sunDir);  // mu in the paper which is the cosine of the angle between the sun direction and the ray direction
    float phaseR = rayleighPhase(mu);
    float phaseM = miePhase(mu);

    for (int i = 0; i < numSamples; ++i)
	{
        vec3 samplePosition = origin + (tCurrent + segmentLength * 0.5) * dir;
        float height = length(samplePosition) - earthRadius;
        // compute optical depth for light
        float hr = exp(-height / Hr) * segmentLength;
        float hm = exp(-height / Hm) * segmentLength;
        opticalDepthR += hr;
        opticalDepthM += hm;
        // light optical depth
        float t0Light, t1Light;
        raySphereIntersect(samplePosition, pc.sunDir, atmosphereRadius, t0Light, t1Light);
        float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0;
        float opticalDepthLightR = 0, opticalDepthLightM = 0;
        int j;
        for (j = 0; j < numSamplesLight; ++j)
		{
            vec3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5) * pc.sunDir;
            float heightLight = length(samplePositionLight) - earthRadius;
            if (heightLight < 0) break;
            opticalDepthLightR += exp(-heightLight / Hr) * segmentLengthLight;
            opticalDepthLightM += exp(-heightLight / Hm) * segmentLengthLight;
            tCurrentLight += segmentLengthLight;
        }
        if (j == numSamplesLight)
		{
            vec3 tau = betaR * (opticalDepthR + opticalDepthLightR) + betaM * 1.1 * (opticalDepthM + opticalDepthLightM);
            vec3 attenuation = vec3(exp(-tau.x), exp(-tau.y), exp(-tau.z));
            sumR += attenuation * hr;
            sumM += attenuation * hm;
        }
        tCurrent += segmentLength;
    }

    return sunIntensity * (sumR * betaR * phaseR + sumM * betaM * phaseM);
}


void main()
{
	vec3 skyColor = computeSkyColor(vec3(0.0, earthRadius + 1.0, 0.0), rayDir, 0.0, 10000000.0);

	// Apply gamma correction
	skyColor.r = skyColor.r < 1.413 ? pow(skyColor.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-skyColor.r);
	skyColor.g = skyColor.g < 1.413 ? pow(skyColor.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-skyColor.g);
	skyColor.b = skyColor.b < 1.413 ? pow(skyColor.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-skyColor.b);

	outColor = vec4(skyColor, 1.0);
}
