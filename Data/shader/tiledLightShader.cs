#version 430
#define NB_MIPMAP 7

#define PI 3.14159265359

#define LIGHT_BIAS 0.1f
#define LS_X 32
#define LS_Y 30

layout(local_size_x=LS_X, local_size_y=LS_Y, local_size_z=1) in;

layout(std140, binding = 0) uniform DrawParameter
{
	mat4 view, proj;
	mat4 projView, invView, invProj, invProjView, worldOriginInvProjView;
	vec4 cameraPos, cameraUp, cameraDir, worldOrigin;
	vec4 time;
}; 

struct Light
{
	vec4 head; // (type, radius, power, x)
	vec4 position;
    vec4 color;
	vec4 spot;
};
layout(std140, binding = 0) buffer lightsBuffer { Light lights[]; };

uniform writeonly image2D image0;

uniform sampler2D texture0; // color
uniform sampler2D texture1; // normal
uniform sampler2D texture2; // material
uniform sampler2D texture3; // depth
uniform sampler2D texture4; // brdf

uniform samplerCube textures[24]; // the first is the global cubemap

uniform int nbLight;

#define MAX_LIGHT_TILE 128
shared uint lightsTile[MAX_LIGHT_TILE];
shared uint lightsTileSize;

vec3 computeFragPosWorldOrigin(float depth, vec2 texCoord)
{
	vec4 position = worldOriginInvProjView * vec4((texCoord.st-vec2(0.5))*2.0, (depth-0.5)*2.0, 1);
	return position.xyz / position.w;
}

vec3 computeFragPos(float depth, vec2 texCoord)
{
	vec4 position = invProjView * vec4((texCoord.st-vec2(0.5))*2.0, (depth-0.5)*2.0, 1);
	return position.xyz / position.w;
}

vec4 computePlan(vec3 p1, vec3 p2, vec3 p3)
{
	vec3 n=normalize(cross(p1 - p2, p3 - p2));
	return vec4(n, -dot(n,p2));
}

vec4 plans[6]; // left right top down near far

int collide(uint light)
{
	for(uint i=0 ; i<4 ; ++i)
	{
		if(dot(plans[i].xyz, lights[light].position.xyz - worldOrigin.xyz) + plans[i].w < -lights[light].head.y)
			return 0;
	}
	return 1;
}

float distPlan(vec4 plan, vec3 p)
{
	return dot(plan.xyz, p) + plan.w;
}

float D_GGX(float rough, float dotNH)
{
	float rough2 = rough*rough;
	float alpha2 = rough2 * rough2;
	float deno = dotNH*dotNH*(alpha2-1)+1;
	return alpha2 / (PI*deno*deno);
}

float helper_G_smith(float dotVal, float k)
{
	return dotVal / (dotVal*(1-k)+k);
}

float G_smith(float dotNV, float dotNL, float roughness)
{
	float k = (roughness+1)*(roughness+1) / 8;
    return helper_G_smith(dotNL,k)*helper_G_smith(dotNV,k);
}

float F_shlick(float F0, float dotVH)
{
	return F0+(1.f-F0)*pow((1.f-dotVH),5);
}

vec3 approximateSpecular(vec3 specColor, float roughness, vec3 r, float dotNV, int indexCubeMap);
vec3 parallaxCorrection(vec3 position, vec3 dir, vec3 center, float radius);

#define LIGHT_THRESHOLD 0.02f

void main()
{
	lightsTileSize = 0;
	barrier();
	
	ivec2 texSize = textureSize(texture0,0);
	if(gl_GlobalInvocationID.x >= texSize.x || gl_GlobalInvocationID.y >= texSize.y)
		return;
	
 	// vec3 color = texelFetch(texture0, ivec2(gl_GlobalInvocationID.xy),0).xyz;
	// vec2 minMaxDepth = texelFetch(texture4, ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y), 0).xy;
	// if(minMaxDepth.x == 1)
	// {
		// imageStore(image0, ivec2(gl_GlobalInvocationID.xy), color);
		// return;
	// }
	
	/*******************/
	/**BUILD TILE STEP**/
	/*******************/
	
	/** Compute frutum plan **/
	#define DEPTH_F -1
	vec4 unprojVec = worldOriginInvProjView*vec4(-1,1,DEPTH_F,1);
	vec3 tl = unprojVec.xyz/unprojVec.w;
	
	unprojVec = worldOriginInvProjView*vec4(-1,-1,DEPTH_F,1);
    vec3 bl = unprojVec.xyz/unprojVec.w;
	
	unprojVec = worldOriginInvProjView*vec4(1,-1,DEPTH_F,1);
    vec3 br = unprojVec.xyz/unprojVec.w;
	
	unprojVec = worldOriginInvProjView*vec4(1,1,DEPTH_F,1);
	vec3 tr = unprojVec.xyz/unprojVec.w;
	
	vec3 step_x = (br-bl) / gl_NumWorkGroups.x;
	vec3 step_y = (tl-bl) / gl_NumWorkGroups.y;
	
	vec3 tile_bl = bl + step_x*gl_WorkGroupID.x + step_y*gl_WorkGroupID.y;
	vec3 tile_br = bl + step_x*(gl_WorkGroupID.x+1) + step_y*gl_WorkGroupID.y;
	vec3 tile_tl = bl + step_x*gl_WorkGroupID.x + step_y*(gl_WorkGroupID.y+1);
	vec3 tile_tr = bl + step_x*(gl_WorkGroupID.x+1) + step_y*(gl_WorkGroupID.y+1);
	
	plans[0] = computePlan(vec3(cameraPos)-vec3(worldOrigin),tile_tl,tile_bl);
	plans[1] = computePlan(vec3(cameraPos)-vec3(worldOrigin),tile_br,tile_tr);
	plans[2] = computePlan(vec3(cameraPos)-vec3(worldOrigin),tile_tr,tile_tl);
	plans[3] = computePlan(vec3(cameraPos)-vec3(worldOrigin),tile_bl,tile_br);
	
	//vec4 pts_near = worldOriginInvProjView*vec4(0,0,minMaxDepth.x*2-1,1);
	//vec4 pts_far = worldOriginInvProjView*vec4(0,0,minMaxDepth.y*2-1,1);
	//plans[4] = computePlan(pts_near.xyz/pts_near.w, cameraDir);
	//plans[5] = computePlan(pts_far.xyz/pts_far.w, -cameraDir.xyz);
	
	uint localRange = 1 + (nbLight / (LS_X*LS_Y));

	uint start = gl_LocalInvocationIndex * localRange;
	uint end = min(start+localRange, nbLight);
	
	uint safe_l=0;

	for(uint i=start ; i<end && safe_l < MAX_LIGHT_TILE ; ++i)
	{
		if(collide(i) == 1)
		{
		    safe_l = atomicAdd(lightsTileSize, 1);
			lightsTile[safe_l] = i;
		}
	}
	
	barrier();

	/*******************/
	/** LIGHTING STEP **/
	/*******************/
	
	float depth = texelFetch(texture3, ivec2(gl_GlobalInvocationID.xy),0).r;
		
	if(depth == 1)
	{
		imageStore(image0, ivec2(gl_GlobalInvocationID.xy), vec4(0,0,0,1));
		return;
	}
	
	vec3 color = texelFetch(texture0, ivec2(gl_GlobalInvocationID.xy),0).xyz;
	vec4 normal_frag = texelFetch(texture1, ivec2(gl_GlobalInvocationID.xy), 0);
	vec4 material = texelFetch(texture2, ivec2(gl_GlobalInvocationID.xy), 0); // roughness, metal like, specular 
	vec3 normal = normalize(normal_frag.xyz*2-1);
	vec3 frag_pos = computeFragPos(depth, vec2(gl_GlobalInvocationID.x/float(texSize.x), gl_GlobalInvocationID.y/float(texSize.y)));
	
	vec3 V = normalize(vec3(cameraPos) - frag_pos);
	float dotNV = clamp(dot(normal, V),0.001,0.999);
	
	float metalic = clamp(material.y,0,1);
	float roughness = clamp(material.x,0,1);
	float specular = material.z;
	int cmAffected = int(specular * 255.f + 0.5);
	cmAffected %= 2;
	
	float accAmbient = 0;
	vec3 accAmbientColor = vec3(0);
	vec3 accLight = vec3(0);
	
	for(int i=0 ; i<lightsTileSize ; ++i)
	{
		vec3 L =(lights[lightsTile[i]].position.xyz - worldOrigin.xyz - frag_pos);
		float dist = length(L);
			
		if(lights[lightsTile[i]].head.x > 0)
		{
			if(cmAffected == 0)
				continue;
			
			float radiusLight = lights[lightsTile[i]].head.y * 0.8;
			float distAtt = lights[lightsTile[i]].head.y * 0.2;
			int indexCubemap = int(lights[lightsTile[i]].head.w)+1;
			
			
			vec3 diffuseTerm = color * (1-material.y) * textureLod(textures[indexCubemap], parallaxCorrection(vec3(0,0,0), normal, L, lights[lightsTile[i]].head.y), NB_MIPMAP-1).xyz;
		
			vec3 specularColor = mix(vec3(specular), vec3(color), metalic);
			vec3 specTerm = approximateSpecular(specularColor, roughness, parallaxCorrection(vec3(0,0,0), reflect(-V, normal), L, lights[lightsTile[i]].head.y), dotNV, indexCubemap);
			
			float att = dist < radiusLight ? 1 : 1 - (min(dist - radiusLight, distAtt) / distAtt);
		
			accAmbient += att;
			accAmbientColor += (specTerm + diffuseTerm) * att;
		}
		else
		{
			float radiusLight = lights[lightsTile[i]].head.y;
			float invPower = 1.f / lights[lightsTile[i]].head.z;
			
			float coef = (1.f/LIGHT_THRESHOLD - invPower) / (radiusLight*radiusLight);
			float att = 1.f/(invPower + coef*dist) - LIGHT_THRESHOLD;
			
			L = normalize(L);

			vec3 H = normalize(L + V);
			
			float dotNL = dot(normal, L);
			float dotNH = dot(normal, H);
			float dotVH = dot(V,H);
			
			if(dotNL <= 0 || dotNV <= 0) continue;
			
			dotNL = clamp(dotNL, 0.001,0.999);
			dotNH = clamp(dotNH, 0.001,0.999);
			dotVH = clamp(dotVH, 0.001,0.999);
			
			float G_CT = G_smith(dotNV, dotNL, roughness);
			float F_CT = F_shlick(mix(0.04, 0.9, metalic), dotVH);
			float D_CT = D_GGX(roughness, dotNH);
			
			float specular = D_CT*G_CT*F_CT / (4*dotNV*dotNL);
			
			vec3 diffuseColor = color - color * metalic;
			vec3 specularColor = mix(vec3(specular), color, metalic);
			
			accLight += att * dotNL * lights[lightsTile[i]].color.xyz * (diffuseColor + specularColor*specular);
		}
	}
	
	if(accAmbient > 1)
		accAmbientColor /= accAmbient;
	else
	{
		vec3 diffuseTerm = color * (1-material.y) * textureLod(textures[0], normal, NB_MIPMAP-1).xyz;
		
		vec3 specularColor = mix(vec3(specular), vec3(color), metalic);
		vec3 specTerm = approximateSpecular(specularColor, roughness, reflect(-V, normal), dotNV, 0);
		accAmbientColor += (specTerm + diffuseTerm) * (1-accAmbient);
	}
	
	imageStore(image0, ivec2(gl_GlobalInvocationID.xy), vec4(accLight+accAmbientColor,1));
	
}

vec3 approximateSpecular(vec3 specColor, float roughness, vec3 r, float dotNV, int indexCubeMap)
{
	const float rough[NB_MIPMAP] = {0, 0.05, 0.13, 0.25, 0.45, 0.66, 1};
	int index=NB_MIPMAP-2;
	for(int i=0 ; i<NB_MIPMAP-1 ; ++i)
	{
		if(rough[i] >= roughness)
		{
			index=i;
			break;
		}
	}
	
	float coef = (roughness - rough[index]) / (rough[index+1] - rough[index]);
	
	vec2 a_b = texture(texture4, vec2(dotNV, roughness)).xy;
	
	return textureLod(textures[indexCubeMap], r, float(index)+coef).rgb * (specColor*a_b.x+vec3(a_b.y));
}

vec3 parallaxCorrection(vec3 position, vec3 dir, vec3 center, float radius)
{
	vec3 boxMax = vec3(center - vec3(radius));
	vec3 boxMin = vec3(center + vec3(radius));
	
	// Following is the parallax-correction code
	// Find the ray intersection with box plane
	vec3 FirstPlaneIntersect =  (boxMax - position) / dir;
	vec3 SecondPlaneIntersect = (boxMin - position) / dir;
	
	// Get the furthest of these intersections along the ray
	// (Ok because x/0 give +inf and -x/0 give –inf )
	vec3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	// Find the closest far intersection
	float dist = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

	// Get the intersection position
	vec3 interectPosition = position + dir * dist;

	return interectPosition - center;
}