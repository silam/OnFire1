#version 150
//You need the texture coordinates to be passed in from the vertex shader (interpolated)

//You also need your uniform sampler so you can use those coordinates to look up a texel
// uniform sampler2D texture;
uniform sampler2D permTexture;
uniform sampler1D simplexTexture;
uniform sampler2D gradTexture;
uniform float time; // Used for texture animation

uniform sampler3D PerlinLookup;
uniform float     FrameCount; 

in vec2 vtexCoord;
in vec3 vtexCoord3D;
//varying vec4 v_color;

out vec4  fColor;



/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another texture size.

/*
 * The interpolation function. This could be a 1D texture lookup
 * to get some more speed, but it's not the main part of the algorithm.
 */
float fade(float t) {
  // return t*t*(3.0-2.0*t); // Old fade, yields discontinuous second derivative
  return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/*
 * 2D simplex noise. Somewhat slower but much better looking than classic noise.
 */
float snoise(vec2 P) {

// Skew and unskew factors are a bit hairy for 2D, so define them as constants
// This is (sqrt(3.0)-1.0)/2.0
#define F2 0.366025403784
// This is (3.0-sqrt(3.0))/6.0
#define G2 0.211324865405

  // Skew the (x,y) space to determine which cell of 2 simplices we're in
 	float s = (P.x + P.y) * F2;   // Hairy factor for 2D skewing
  vec2 Pi = floor(P + s);
  float t = (Pi.x + Pi.y) * G2; // Hairy factor for unskewing
  vec2 P0 = Pi - t; // Unskew the cell origin back to (x,y) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec2 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 2D case, the simplex shape is an equilateral triangle.
  // Find out whether we are above or below the x=y diagonal to
  // determine which of the two triangles we're in.
  vec2 o1;
  if(Pf0.x > Pf0.y) o1 = vec2(1.0, 0.0);  // +x, +y traversal order
  else o1 = vec2(0.0, 1.0);               // +y, +x traversal order

  // Noise contribution from simplex origin
  vec2 grad0 = texture2D(permTexture, Pi).rg * 4.0 - 1.0;
  float t0 = 0.5 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from middle corner
  vec2 Pf1 = Pf0 - o1 + G2;
  vec2 grad1 = texture2D(permTexture, Pi + o1*ONE).rg * 4.0 - 1.0;
  float t1 = 0.5 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from last corner
  vec2 Pf2 = Pf0 - vec2(1.0-2.0*G2);
  vec2 grad2 = texture2D(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float t2 = 0.5 - dot(Pf2, Pf2);
  float n2;
  if(t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 70.0 * (n0 + n1 + n2);
}

/*
 * 2D classic Perlin noise. Fast, but less useful than 3D noise.
 */
float noise(vec2 P)
{
  vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
  vec2 Pf = fract(P);             // Fractional part for interpolation

  // Noise contribution from lower left corner
  vec2 grad00 = texture2D(permTexture, Pi).rg * 4.0 - 1.0;
  float n00 = dot(grad00, Pf);

  // Noise contribution from lower right corner
  vec2 grad10 = texture2D(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
  float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

  // Noise contribution from upper left corner
  vec2 grad01 = texture2D(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
  float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

  // Noise contribution from upper right corner
  vec2 grad11 = texture2D(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

  // Blend contributions along x
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));

  // Blend contributions along y
  float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));

  // We're done, return the final noise value.
  return n_xy;
}

/*
 * 4D simplex noise. A lot faster than classic 4D noise, and better looking.
 */

float snoise(vec4 P) {

// The skewing and unskewing factors are hairy again for the 4D case
// This is (sqrt(5.0)-1.0)/4.0
#define F4 0.309016994375
// This is (5.0-sqrt(5.0))/20.0
#define G4 0.138196601125

  // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
 	float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
  vec4 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
  vec4 P0 = Pi - t; // Unskew the cell origin back to (x,y,z,w) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec4 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
  // To find out which of the 24 possible simplices we're in, we need to
  // determine the magnitude ordering of x, y, z and w components of Pf0.
  // The method below is presented without explanation. It uses a small 1D
  // texture as a lookup table. The table is designed to work for both
  // 3D and 4D noise and contains 64 indices, of which only 24 are actually
  // used. An extension to 5D would require a larger texture here.
  float c1 = (Pf0.x > Pf0.y) ? 0.5078125 : 0.0078125; // 1/2 + 1/128
  float c2 = (Pf0.x > Pf0.z) ? 0.25 : 0.0;
  float c3 = (Pf0.y > Pf0.z) ? 0.125 : 0.0;
  float c4 = (Pf0.x > Pf0.w) ? 0.0625 : 0.0;
  float c5 = (Pf0.y > Pf0.w) ? 0.03125 : 0.0;
  float c6 = (Pf0.z > Pf0.w) ? 0.015625 : 0.0;
  float sindex = c1 + c2 + c3 + c4 + c5 + c6;



  // Si Lam Remvove this
  vec4 offsets; // = texture1D(simplexTexture, sindex).rgba; // texture1D
  vec4 o1 = step(0.625, offsets);
  vec4 o2 = step(0.375, offsets);
  vec4 o3 = step(0.125, offsets);

  // Noise contribution from simplex origin
  float perm0xy = texture2D(permTexture, Pi.xy).a;
  float perm0zw = texture2D(permTexture, Pi.zw).a;
  vec4  grad0 = texture2D(gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec4 Pf1 = Pf0 - o1 + G4;
  o1 = o1 * ONE;
  float perm1xy = texture2D(permTexture, Pi.xy + o1.xy).a;
  float perm1zw = texture2D(permTexture, Pi.zw + o1.zw).a;
  vec4  grad1 = texture2D(gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from third corner
  vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
  o2 = o2 * ONE;
  float perm2xy = texture2D(permTexture, Pi.xy + o2.xy).a;
  float perm2zw = texture2D(permTexture, Pi.zw + o2.zw).a;
  vec4  grad2 = texture2D(gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  
  // Noise contribution from fourth corner
  vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
  o3 = o3 * ONE;
  float perm3xy = texture2D(permTexture, Pi.xy + o3.xy).a;
  float perm3zw = texture2D(permTexture, Pi.zw + o3.zw).a;
  vec4  grad3 = texture2D(gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if (t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }
  
  // Noise contribution from last corner
  vec4 Pf4 = Pf0 - vec4(1.0-4.0*G4);
  float perm4xy = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  float perm4zw = texture2D(permTexture, Pi.zw + vec2(ONE, ONE)).a;
  vec4  grad4 = texture2D(gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
  float t4 = 0.6 - dot(Pf4, Pf4);
  float n4;
  if(t4 < 0.0) n4 = 0.0;
  else {
    t4 *= t4;
    n4 = t4 * t4 * dot(grad4, Pf4);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */
float snoise(vec3 P) {

// The skewing and unskewing factors are much simpler for the 3D case
#define F3 0.333333333333
#define G3 0.166666666667

  // Skew the (x,y,z) space to determine which cell of 6 simplices we're in
 	float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
  vec3 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z) * G3;
  vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
  // To find out which of the six possible tetrahedra we're in, we need to
  // determine the magnitude ordering of x, y and z components of Pf0.
  // The method below is explained briefly in the C code. It uses a small
  // 1D texture as a lookup table. The table is designed to work for both
  // 3D and 4D noise, so only 8 (only 6, actually) of the 64 indices are
  // used here.
  float c1 = (Pf0.x > Pf0.y) ? 0.5078125 : 0.0078125; // 1/2 + 1/128
  float c2 = (Pf0.x > Pf0.z) ? 0.25 : 0.0;
  float c3 = (Pf0.y > Pf0.z) ? 0.125 : 0.0;
  float sindex = c1 + c2 + c3;
  vec3 offsets;// = texture1D(simplexTexture, sindex).rgb;
  vec3 o1 = step(0.375, offsets);
  vec3 o2 = step(0.125, offsets);

  // Noise contribution from simplex origin
  float perm0 = texture2D(permTexture, Pi.xy).a;
  vec3  grad0 = texture2D(permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec3 Pf1 = Pf0 - o1 + G3;
  float perm1 = texture2D(permTexture, Pi.xy + o1.xy*ONE).a;
  vec3  grad1 = texture2D(permTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from third corner
  vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
  float perm2 = texture2D(permTexture, Pi.xy + o2.xy*ONE).a;
  vec3  grad2 = texture2D(permTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  
  // Noise contribution from last corner
  vec3 Pf3 = Pf0 - vec3(1.0-3.0*G3);
  float perm3 = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  vec3  grad3 = texture2D(permTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if(t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 32.0 * (n0 + n1 + n2 + n3);
}

/*****************************************************************************
 *                                                                           *
 *****************************************************************************/
float perlin_turbulence(in vec3 vec, in float freq)
{
  float t;
  for (t = 0.0; freq >= 1.0; freq *= 0.5)
    {
      t += abs(snoise(freq * vec)) / freq;
    }
  return t;
}


/*****************************************************************************
 *                                                                           *
 *****************************************************************************/
vec3 fade(vec3 t)
{ 
  return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); 
}





void main()
{
	//v_color = vec4(1,1,1,1);
	//float n = snoise(vec4(4.0 * v_texCoord3D.xyz, 0.5 * time));
	//gl_FragColor = v_color * vec4(0.5 + 0.5 * vec3(n, n, n), 1.0);


	//float n = noise(v_texCoord2D * 32.0 + 240.0);
    //float n = snoise(v_texCoord2D * 16.0);
    
	//float n = noise(vec3(4.0 * vtexCoord.xy * (2.0 + sin(0.5 * time))));
    
	//float sine = (2.0 + sin(0.5 * time));
	//float n = snoise(	vec3(32.0 * vec3(vtexCoord.xy,1) * sine ));
    
	//float n = noise(vec4(8.0 * v_texCoord3D.xyz, 0.5 * time));
    //float n = snoise(vec4(4.0 * v_texCoord3D.xyz, 0.5 * time));

	 //float n = snoise(vtexCoord * 64.0 );
	 float n = noise(vtexCoord * 32.0 + 240.0);
	 
	 //float n = snoise(vec4(80.0 * vtexCoord.xy, time , time));
	 //vec4 x_color = vec4(1,0,0,1);

     //fColor =  x_color*vec4(0.5 + 0.5 * vec3(n, n, n), 1.0);

	

	//you need to look up the appropriate color for this fragment based on the texture map and the texture cordinates
	fColor = texture2D(permTexture,  vtexCoord.st );// + vec2(cos( vtexCoord.s), cos( vtexCoord.s) ));
	
	//float i = texture2D(_IDL_ImageTexture, gl_TexCoord[0].xy).r;  
    //gl_FragColor = texture1D(lut, i);  


	//gl_FragColor = texture2D(ImageTexture, gl_TexCoord[0].st + 	vec2(0.5, cos( gl_TexCoord[0].s)));

	/*
	int frame = 30;

	const float zoom = 2.0;
	const float freq = 3.9;
	
	vec3 point = 2.0 * vec3(vtexCoord, 1);
	float radius = length(point);
	
	float dr = perlin_turbulence(zoom * (point + vec3(0.0, 0.0, frame)), freq);
	
	vec4 texel;
	texel.r = clamp(1.0 - 1.0 * pow(radius + dr / 3, 5), 0, 1);
	texel.g = clamp(1.0 - 2.0 * pow(radius + dr / 3, 3), 0, 1);
	texel.b = 0.9;
	texel.a = 1; //length(texel.rgb);
	
	fColor = texel;
	*/

}



/*
void main()
{
	

	//you need to look up the appropriate color for this fragment based on the texture map and the texture cordinates
	fColor = texture2D(texture, vtexCoord);
	

}
*/