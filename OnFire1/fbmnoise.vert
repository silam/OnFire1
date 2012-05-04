#version 150

// FIRE
uniform float time;
varying vec3 vTexCoord3D;


in vec4 vPosition; 
in vec2 texCoord;
out vec3 position;
//you'll need to pass the texture coordinates over to the fragment shader, so you'll need an out

in float vtime;
out float ftime;
uniform float  uStability;
out float vStability;

uniform float utime;
uniform mat4 model_view;
uniform mat4 projection;

out vec2 vtexCoord;
out vec3 vtexCoord3D;

void main(void) {
	
	
	vec4 veyepos = model_view*vPosition;
	
	//don't forget to pass your texture coordinate through!
	
	vtexCoord3D = vPosition.xyz * 4 + vec3(0.0, 0.0, vtime);

	ftime = vtime;
	// position = veyepos.xyz  * 4 + vec3(0.0, 0.0, vtime);
	vtexCoord = texCoord;// + vec2(0.0, utime );
	ftime = utime;
	vStability = uStability;
	gl_Position = projection * veyepos;

	
	//vTexCoord3D = gl_Vertex.xyz * 2.0 + vec3(0.0, 0.0, -time);
	//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
