#version 450
//Vertex attributes
layout(location = 0) in vec3 vPos; // Vertex position in model space
layout(location = 1) in vec3 vNormal; // Vertex normal in model space
layout(location = 2) in vec2 vTexCoord; // vertex texture coordinate (UV)
uniform mat4 _Model; // Model->World Matrix
uniform mat4 _ViewProjection; // Combined View-> Projection matrix
out vec3 Normal; // Output for next shader
// this whole block will be passed to the next shader stage
out Surface
{
	vec3 WorldPos; // vertex position in world space
	vec3 WorldNormal; // vertex normal in world space
	vec2 TexCoord;
}vs_out;
void main()
{
	// transform vertex position to world space
	vs_out.WorldPos = vec3(_Model * vec4(vPos,1.0));
	// transform vertex normal to world space using normal matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;
	vs_out.TexCoord = vTexCoord;
	// Transform vertex position to homogeneous clip space
	gl_Position = _ViewProjection * _Model * vec4(vPos,1.0);
}