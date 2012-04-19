#version 150

in vec4 vPosition; 
in vec2 texCoord;

//you'll need to pass the texture coordinates over to the fragment shader, so you'll need an out

uniform mat4 model_view;
uniform mat4 projection;

out vec2 vtexCoord;
out vec3 vtexCoord3D;
varying vec4 v_color;

void
main()
{	
	
	
	//don't forget to pass your texture coordinate through!
	vtexCoord = texCoord;
	vtexCoord3D = vPosition.xyz;

	v_color = vec4(1, 1, 1, 1);
	gl_Position = projection * model_view*vPosition;
	

}