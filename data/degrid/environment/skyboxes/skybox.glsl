
$include /cage/shader/shaderConventions.h

$include /cage/shader/engine/vertex.glsl

void main()
{
	varInstanceId = gl_InstanceID;
	varUv = inUv;
	vec3 p3 = mat3(uniViewport.vMat) * mat3(uniMeshes[varInstanceId].mMat) * vec3(inPosition);
	gl_Position = uniViewport.pMat * vec4(p3, 1);
}

$include /cage/shader/engine/fragment.glsl

void main()
{
	Material material = loadMaterial();
	outColor.rgb = material.albedo;
	outColor.a = material.opacity;
}
