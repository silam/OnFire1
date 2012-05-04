#version 150

// FIRE
uniform float time;


in vec4 vPosition; 
in vec2 texCoord;
out vec3 position;
//you'll need to pass the texture coordinates over to the fragment shader, so you'll need an out

in float vtime;
out float ftime;
uniform float  uStability;
out float vStability;

uniform int  uRoughness;
flat out int vRoughness;

uniform float utime;
uniform mat4 model_view;
uniform mat4 projection;

out vec2 vtexCoord;
out vec3 vtexCoord3D;

void main(void) {
	
	
	vec4 veyepos = model_view*vPosition;
	
	//don't forget to pass your texture coordinate through!
		
	ftime = vtime;

	vtexCoord = texCoord;
	ftime = utime;
	vStability = uStability;
	vRoughness = uRoughness;
	gl_Position = projection * veyepos;

}
