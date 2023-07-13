#ifndef LIGHTING
#define LIGHTING

float D_GGX_TR(float ndoth, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float nom = a2;
	float denom = ndoth * ndoth * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return nom / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float dotp, float k)
{
	float nom = dotp;
	float denom = dotp * (1.0 - k) + k;
	return nom / denom;
}

float GeometrySmith(float ndotv, float ndotl, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0f;

	return GeometrySchlickGGX(ndotv, k) * GeometrySchlickGGX(ndotl, k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;

	vec3 N = normalize(normal);
	vec3 T = normalize(tangent);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

vec3 blin_phong()
{
	vec3 color = pow(texture(colorSampler, fragTexCoord).rgb, vec3(2.2));

	//vec3 N = normalize(normal);

	vec3 N = calculateNormal();

	vec3 L = normalize(uboParam.lights[0].xyz - worldPos);
	vec3 V = normalize(ubo.viewPos.xyz - worldPos);
	
	vec3 R = reflect(L, N);
	vec3 diffuse = max(dot(N, L), 0.15) * color.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	return diffuse + specular;
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilterCubeMapSampler, R, lodf).rgb;
	vec3 b = textureLod(prefilterCubeMapSampler, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 pbr(float shadow)
{
	vec3 albedo = pow(texture(colorSampler, fragTexCoord).rgb, vec3(2.2)); // error

	vec2 roughMetalic = texture(roughnessSampler, fragTexCoord).gb;
	float roughness = roughMetalic.x;
	float metallic = roughMetalic.y;

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 texnormal = calculateNormal();

	vec3 n = texnormal;
	vec3 v = normalize(ubo.viewPos.xyz - worldPos);
	float ndotv = clamp(dot(n, v), 0.0, 1.0);
	vec3 f = fresnelSchlick(ndotv, F0);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 1; i ++)
	{
		vec3 l = normalize(uboParam.lights[i].xyz - worldPos);
		float ndotl = dot(n, l);
		if(ndotl > 0.0)
		{
			ndotl = clamp(ndotl, 0.0, 1.0);
			vec3 h = normalize(v + l);

			float ndoth = clamp(dot(n, h), 0.0, 1.0);

			float ndf = D_GGX_TR(ndoth, roughness);
			float g = GeometrySmith(ndotv, ndotl, roughness);

			vec3 nom = ndf * g * f;
			float denom = 4 * ndotv * ndotl + 0.001;
			vec3 specular = nom / denom;

			vec3 ks = f;
			vec3 kd = (vec3(1.0)-f);
			kd *= (1 - metallic);
	
			Lo += (kd * albedo / PI + specular)* ndotl;
		}
	}

	vec3 F = F_SchlickR(ndotv, F0, roughness);
	vec3 Kd = vec3(1.0) - F;
	Kd *= 1.0 - metallic;
	vec3 irradiance = texture(irradianceCubeMapSampler, n).rgb;
	vec3 diffuse = irradiance * albedo;

	vec3 R = reflect(-v, n);
	vec3 prefilterLight = prefilteredReflection(R, roughness);

	vec2 brdflut = texture(BRDFLutSampler, vec2(ndotv, (roughness))).xy;
	vec3 specular = prefilterLight * (F0 * brdflut.x + brdflut.y);

	vec3 ambient = Kd * diffuse + specular;

	vec3 emissive = texture(emissiveSampler, fragTexCoord).rgb * materialData.emissiveFactor;

	vec3 color = Lo * shadow + ambient + emissive;

	return color;
}

#endif