#version 430
#ifdef USE_BINDLESS
#extension GL_ARB_bindless_texture : enable
#endif

struct Material
{
	uvec2 header;
	uvec2 tex0;
	uvec2 tex1;
	uvec2 tex2;
	vec4 parameter;
	uvec4 color_scale_ca_unsused; // ca == cubeMapAffected
};

#ifdef WATER_SHADER
layout(std140, binding = 0) uniform DrawParameter
{
	mat4 view, proj;
	mat4 projView, invView, invProj, invViewProj, worldOriginInvViewProj;
	vec4 cameraPos, cameraUp, cameraDir, worldOrigin;
	vec4 time;
}; 
#endif

vec4 unpackColor(uint col)
{
	uint r_i = (col & 0xff000000)  >> 24;
    uint g_i = (col & 0x00ff0000)  >> 16;
    uint b_i = (col & 0x0000ff00)  >> 8;
    uint a_i = (col & 0x000000ff);
	vec4 rgba;
    rgba.r = r_i / 255.f;
    rgba.g = g_i / 255.f;
    rgba.b = b_i / 255.f;
    rgba.a = a_i / 255.f;
	return rgba;
}


layout(std140, binding = 2) uniform Materials
{
	Material materials[MAX_UBO_VEC4 / 4];
};

smooth in vec2 tCoord;
flat in int v_drawId;
smooth in vec3 v_normal;
smooth in vec3 v_tangent;
#ifdef TRIPLANAR
smooth in vec3 v_worldPos;
#endif
  
layout(location=0) out vec4 outColor; 
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outMaterial;

#ifdef USE_BINDLESS
#define MATERIAL_TEX0 sampler2D(materials[v_drawId].tex0)
#define MATERIAL_TEX1 sampler2D(materials[v_drawId].tex1)
#define MATERIAL_TEX2 sampler2D(materials[v_drawId].tex2)
#else
// Material textures are bound to units 0..2 before each draw call (MeshRenderer)
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
#define MATERIAL_TEX0 texture0
#define MATERIAL_TEX1 texture1
#define MATERIAL_TEX2 texture2
#endif

#ifdef TRIPLANAR
// Triplanar mapping: the textures are projected along the 3 world axes and blended using the surface normal,
// the texture scale is the number of repetitions per world unit.
// Each projection is a non mirrored uv frame (texture up along +Z on the sides) with the same tangent space
// convention as the uv mapped path, so the same textures and normal maps look the same with both shaders.

const float TRIPLANAR_SHARPNESS = 4.0;     // higher values give narrower transitions between projections
const float TRIPLANAR_MIN_WEIGHT = 0.05;   // projections under this weight are not sampled

struct TriplanarProjection
{
	vec2 uv, uvDx, uvDy;
	mat3 tbn;
	float weight;
};

TriplanarProjection projections[3];

TriplanarProjection makeProjection(vec3 t, vec3 b, vec3 axis, float weight, vec3 pos, vec3 posDx, vec3 posDy)
{
	TriplanarProjection p;
	p.uv = vec2(dot(pos, t), dot(pos, b));
	p.uvDx = vec2(dot(posDx, t), dot(posDx, b));
	p.uvDy = vec2(dot(posDy, t), dot(posDy, b));
	p.tbn = mat3(t, cross(t, axis), axis);
	p.weight = weight;
	return p;
}

// pos: world position multiplied by the texture scale, n: normalized surface normal
void setupTriplanar(vec3 pos, vec3 n)
{
	// Explicit gradients from the continuous position: projections are sampled conditionally
	// and the uv flips below would break implicit derivatives.
	vec3 posDx = dFdx(pos);
	vec3 posDy = dFdy(pos);

	vec3 w = pow(abs(n), vec3(TRIPLANAR_SHARPNESS));
	w /= w.x + w.y + w.z;
	w = max(w - TRIPLANAR_MIN_WEIGHT, 0.0);
	w /= w.x + w.y + w.z;

	vec3 s = step(0.0, n) * 2.0 - 1.0;
	projections[0] = makeProjection(vec3(0, s.x, 0),  vec3(0, 0, 1), vec3(s.x, 0, 0), w.x, pos, posDx, posDy);
	projections[1] = makeProjection(vec3(-s.y, 0, 0), vec3(0, 0, 1), vec3(0, s.y, 0), w.y, pos, posDx, posDy);
	projections[2] = makeProjection(vec3(s.z, 0, 0),  vec3(0, 1, 0), vec3(0, 0, s.z), w.z, pos, posDx, posDy);
}

vec4 triplanarTexture(sampler2D tex)
{
	vec4 res = vec4(0);
	for(int i=0 ; i<3 ; ++i)
	{
		if(projections[i].weight > 0.0)
			res += textureGrad(tex, projections[i].uv, projections[i].uvDx, projections[i].uvDy) * projections[i].weight;
	}
	return res;
}

// Reoriented normal mapping: applies the tangent space normal 'detail' on top of the normal 'base'
vec3 blendRNM(vec3 base, vec3 detail)
{
	base.z += 1.0;
	detail.xy = -detail.xy;
	return base * dot(base, detail) / base.z - detail;
}

// n: normalized surface normal
vec3 triplanarNormal(sampler2D normalMap, vec3 n)
{
	vec3 res = vec3(0);
	for(int i=0 ; i<3 ; ++i)
	{
		if(projections[i].weight > 0.0)
		{
			vec3 detail = textureGrad(normalMap, projections[i].uv, projections[i].uvDx, projections[i].uvDy).xyz*2-1;
			// Blended onto the surface normal rather than the projection axis, keeps curved surfaces smooth
			vec3 base = n * projections[i].tbn;
			res += projections[i].tbn * blendRNM(base, detail) * projections[i].weight;
		}
	}
	return res;
}
#endif

void main()  
{  	

	vec4 texColor = vec4(1);
	vec3 n = v_normal;
	vec4 material_tex = vec4(1,1,1,1);
	
	if(materials[v_drawId].header.y > 0)
	{
		float texScale = materials[v_drawId].color_scale_ca_unsused.y / 1000.f;
	#ifdef TRIPLANAR
		n = normalize(n);
		setupTriplanar(v_worldPos * texScale, n);
		texColor = triplanarTexture(MATERIAL_TEX0);
	#else
		texColor = texture(MATERIAL_TEX0, tCoord * texScale);
	#endif
		
	#ifdef ALPHA_TEST
		if(texColor.a < 0.5) discard;
	#endif
	
		if(materials[v_drawId].header.y > 1)
		{
		#ifdef WATER_SHADER
			vec2 dirtex = (materials[v_drawId].parameter.xy-vec2(0.5))*4;
			vec2 dirtex2 = vec2(cos(dirtex.y), sin(dirtex.y));
			dirtex = vec2(cos(dirtex.x), sin(dirtex.x));
			
			float scaleTime = materials[v_drawId].parameter.z * 0.05;
			vec3 n1 = texture(MATERIAL_TEX1, (tCoord * texScale * 20 + dirtex*time.x*scaleTime)).xyz*2-1;
			vec3 n2 = texture(MATERIAL_TEX1, (tCoord * texScale * 20 + dirtex2*time.x*scaleTime)).xyz*2-1;
			n = (n1+n2) * 0.5;
			n.z *= materials[v_drawId].parameter.w * 10;
		#else
			#ifdef TRIPLANAR
			n = triplanarNormal(MATERIAL_TEX1, n);
			#else
			vec3 t = normalize(v_tangent);
			mat3 tbn = mat3(t, cross(t,n), n);
			
			n = tbn*(texture(MATERIAL_TEX1, tCoord * texScale).xyz*2-1);
			#endif
			
			if(materials[v_drawId].header.y > 2)
			{
				#ifdef TRIPLANAR
				material_tex = triplanarTexture(MATERIAL_TEX2);
				#else
				material_tex = texture(MATERIAL_TEX2, tCoord * texScale);
				#endif
			}
			
		#endif
		}
	}
	n = normalize(n);
	
	outColor = texColor * unpackColor(materials[v_drawId].color_scale_ca_unsused.x);
	outNormal = vec4(n*0.5+0.5, 1);
	#ifdef PORTAL_SHADER
	outMaterial = vec4(1, 1, materials[v_drawId].color_scale_ca_unsused.y / 1000.f,0);
	#else
	#ifdef WATER_SHADER
	outMaterial = vec4(0, 1, 1,0);
	#else
	outMaterial = vec4(materials[v_drawId].parameter) * material_tex;
	int spec = int(outMaterial.z * 255.f + 0.5);
	spec = (spec >> 1) << 1;
	spec += materials[v_drawId].color_scale_ca_unsused.z>0 ? 1:0;
	outMaterial.z = spec / 255.f;
	#endif
	#endif
}