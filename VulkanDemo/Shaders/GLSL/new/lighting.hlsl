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

//https://blog.selfshadow.com/publications/s2017-shading-course/imageworks/s2017_pbs_imageworks_slides_v2.pdf
vec3 AverageFresnel(vec3 r, vec3 g)
{
	return vec3(0.087237) + 0.0230685 * g - 0.0864902 * g * g + 0.0774594 * g * g * g
		+ 0.782654 * r - 0.136432 * r * r + 0.278708 * r * r * r
		+ 0.19744 * g * r + 0.0360605 * g * g * r - 0.2586 * g * r * r;
}

vec3 MultiScatterBRDF(vec3 N, vec3 L, vec3 V, vec3 albedo, float roughness)
{
	float ndotl = clamp(dot(N, L), 0.0, 1.0);
	float ndotv = clamp(dot(N, V), 0.0, 1.0);

	vec3 Eo = texture(EmuSampler, vec2(ndotv, roughness)).rgb * 0.5;
	vec3 Ei = texture(EmuSampler, vec2(ndotl, roughness)).rgb * 0.5;

	vec3 Eavg = texture(EavgSampler, vec2(0.0, roughness)).rgb * 0.5;

	vec3 edgetint = vec3(0.827, 0.792, 0.678);
	vec3 Favg = AverageFresnel(albedo, edgetint);

	vec3 fms = (vec3(1.0) - Ei) * (vec3(1.0) - Eo) / (PI * (vec3(1.0) - Eavg));
	vec3 fadd = Favg * Eavg / (vec3(1.0) - Favg * (vec3(1.0) - Eavg));

	return fms * fadd;
}

vec3 DirectLighting(vec3 n, vec3 v, vec3 albedo, vec3 F0, float roughness, float metallic)
{
	float ndotv = clamp(dot(n, v), 0.0, 1.0);
	vec3 f = fresnelSchlick(ndotv, F0);

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 1; i++)
	{
		vec3 l = normalize(uboParam.lights[i].xyz - worldPos);
		float ndotl = dot(n, l);
		if (ndotl > 0.0)
		{
			ndotl = clamp(ndotl, 0.0, 1.0);
			vec3 h = normalize(v + l);

			float ndoth = clamp(dot(n, h), 0.0, 1.0);

			float ndf = D_GGX_TR(ndoth, roughness);
			float g = GeometrySmith(ndotv, ndotl, roughness);

			vec3 nom = ndf * g * f;
			float denom = 4 * ndotv * ndotl + 0.001;
			vec3 specular = nom / denom;

			//vec3 ks = f;
			//vec3 kd = (vec3(1.0) - f);
			//kd *= (1 - metallic);

			//Lo += (kd * albedo / PI + specular) * ndotl;
			
			vec3 Fms = MultiScatterBRDF(n, l, v, albedo, roughness);
            Lo += (specular + Fms) * ndotl;
        }
	}

	return Lo;
}

#ifdef IBLLIGHTING

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

vec3 IBLIndirectLighting(vec3 n, vec3 v, vec3 albedo, vec3 F0, float roughness, float metallic)
{
	float ndotv = clamp(dot(n, v), 0.0, 1.0);
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

	return ambient;
}

#endif

#ifdef RSMLIGHTING

vec3 RSMLighting(vec3 worldPos, vec3 N, vec3 albedo)
{
	vec3 coords = shadowCoord.xyz;
	coords.xyz /= shadowCoord.w;
	vec2 uv = coords.xy * 0.5 + 0.5;

	float radius = random.xi[0].z;
	vec3 E = vec3(0.0);
	for (int i = 0; i < RSM_SAMPLE_COUNT; i++)
	{
		vec2 r = random.xi[i].xy;
		vec2 offset = vec2(radius * r.x * sin(PI2 * r.y), radius * r.x * cos(PI2 * r.y));
		vec2 sampleCoords = uv + offset;

		vec3 pos = texture(vPosSampler, sampleCoords).xyz;
		vec3 normal = texture(vNormalSampler, sampleCoords).xyz;
		vec3 flux = texture(vFluxSampler, sampleCoords).xyz;

		normal = normalize(normal);

		vec3 wi = normalize(pos - worldPos);
		vec3 wo = normalize(worldPos - pos);

		E += flux * max(dot(normal, wo), 0.0) * max(dot(N, wi), 0.0) * r.x * r.x;
	}

	E /= RSM_SAMPLE_COUNT;
	return E * albedo / PI;
}

#endif

vec3 Lighting(float shadow)
{
	vec3 albedo = pow(texture(colorSampler, fragTexCoord).rgb, vec3(2.2)); // error
    //vec3 albedo = texture(colorSampler, fragTexCoord).rgb; 

	vec2 roughMetalic = GetRoughnessAndMetallic();
	float roughness = roughMetalic.x;
	float metallic = roughMetalic.y;

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 n = calculateNormal();
	vec3 v = normalize(ubo.viewPos.xyz - worldPos);

	vec3 Lo = DirectLighting(n, v, albedo, F0, roughness, metallic);

	vec3 ambient = vec3(0.0);
	vec3 emissive = texture(emissiveSampler, fragTexCoord).rgb * materialData.emissiveFactor;

	#ifdef IBLLIGHTING
	ambient = IBLIndirectLighting(n, v, albedo, F0, roughness, metallic);
	#endif

	float ao = 1.0;
	#ifdef SSAO
	
	ivec2 screenSize = textureSize(ssaoSampler, 0);
	vec2 uv = vec2(gl_FragCoord.x / float(screenSize.x), gl_FragCoord.y / float(screenSize.y));
	ao = texture(ssaoSampler, uv).r;

	#endif

    vec3 color = Lo * shadow + ambient + emissive;

    color *= ao;
	
	return color;
}

#endif