/*
 *This C program is an exercise in texture loading
 *user rotation and translation enabled
 * Nathan Gossett
 *Spring 2012
 **/

#include <stdio.h>
#include <GL/Angel.h>
#include <stdlib.h>
#include <math.h>
#include <IL/il.h> //notice the OpenIL include
#include "matrix_stack.h"

#pragma comment(lib, "glew32.lib")
//We have additional libraries to link to now as well
#pragma comment(lib,"ILUT.lib")
#pragma comment(lib,"DevIL.lib")
#pragma comment(lib,"ILU.lib")

typedef unsigned char byte;

matrix_stack stack;

/////////////////////////////

#define B 0x100
#define BM 0xff
#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

#define s_curve(t) ( t * t * (3. - 2. * t) )
#define lerp(t, a, b) ( a + t * (b - a) )
#define setup(i,b0,b1,r0,r1)\
        t = vec[i] + N;\
        b0 = ((int)t) & BM;\
        b1 = (b0+1) & BM;\
        r0 = t - (int)t;\
        r1 = r0 - 1.;
#define at2(rx,ry) ( rx * q[0] + ry * q[1] )
#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )



static int perm[B + B + 2];
static double g3[B + B + 2][3];
static double g2[B + B + 2][2];
static double grad1[B + B + 2];
static int start = 1;

GLint location_time = -1;
GLint location_Stability = -1;
static char noisetypestring[200];
vec4 squareverts[6];
vec2 texcoords[6];

void initperm(void)
{
   int i, j, k;

   for (i = 0 ; i < B ; i++) {
      perm[i] = i;
      grad1[i] = (double)((rand() % (B + B)) - B) / B;

     /* for (j = 0 ; j < 2 ; j++)
         g2[i][j] = (double)((random() % (B + B)) - B) / B;
      normalize2(g2[i]);*/

      /*for (j = 0 ; j < 3 ; j++)
         g3[i][j] = (double)((random() % (B + B)) - B) / B;
      normalize3(g3[i]);*/
   }

   while (--i) {
      k = perm[i];
      perm[i] = perm[j = rand() % B];
      perm[j] = k;
   }

   for (i = 0 ; i < B + 2 ; i++) {
      perm[B + i] = perm[i];
      grad1[B + i] = grad1[i];

      /*for (j = 0 ; j < 2 ; j++)
         g2[B + i][j] = g2[i][j];
      for (j = 0 ; j < 3 ; j++)
         g3[B + i][j] = g3[i][j];*/
   }
}


double noise1(double arg)
{
   int bx0, bx1;
   double rx0, rx1, sx, t, u, v, vec[1];

   vec[0] = arg;
   if (start) {
      start = 0;
      initperm();
   }

   setup(0,bx0,bx1,rx0,rx1);

   sx = s_curve(rx0);
   u = rx0 * grad1[ perm[ bx0 ] ];
   v = rx1 * grad1[ perm[ bx1 ] ];

   return(lerp(sx, u, v));
}



double PerlinNoise1D(double x,double alpha,double beta,int n)
{
   int i;
   double val,sum = 0;
   double p,scale = 1;

   p = x;
   for (i=0;i<n;i++) {
      val = noise1(p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   return(sum);
}

/////////////////////////////
#define WIDTH 500
#define HEIGHT 400
#define TRUE 1
#define FALSE 0


int right_button_down = FALSE;
int left_button_down = FALSE;

int prevMouseX;
int prevMouseY;

double view_rotx = 180.0;
double view_roty = 0.0;
double view_rotz = 0.0;
double z_distance;

//We need three texture files
static GLuint texName[3];


GLuint vao[1];
GLuint vbo[3];

//our modelview and perspective matrices
mat4 mv, p;

//and we'll need pointers to our shader variables
GLuint model_view;
GLuint projection;
GLuint vPosition;
GLuint texCoord;
GLuint texMap;

GLuint programObj, programsimplex, programclassic;

int multiflag = 0;

float Stability;

//Modified slightly from the OpenIL tutorials
ILuint loadTexFile(const char* filename){
	
	ILboolean success; /* ILboolean is type similar to GLboolean and can equal GL_FALSE (0) or GL_TRUE (1)
    it can have different value (because it's just typedef of unsigned char), but this sould be
    avoided.
    Variable success will be used to determine if some function returned success or failure. */


	/* Before calling ilInit() version should be checked. */
	  if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	  {
		/* wrong DevIL version */
		printf("Wrong IL version");
		exit(1);
	  }
 
	  success = ilLoadImage((LPTSTR)filename); /* Loading of image from file */
	if (success){ /* If no error occured: */
		//We need to figure out whether we have an alpha channel or not
		  if(ilGetInteger(IL_IMAGE_BPP) == 3){
			success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE); /* Convert every color component into
		  unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
		  }else if(ilGetInteger(IL_IMAGE_BPP) == 4){
			  success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		  }else{
			  success = false;
		  }
		if (!success){
		  /* Error occured */
		 printf("failed conversion to unsigned byte");
		 exit(1);
		}
	}else{
		/* Error occured */
	   printf("Failed to load image ");
	   printf(filename);
		exit(1);
	}
}

void setupShader(GLuint prog,GLuint vao[1], GLuint vbo[3])
{
	
	glUseProgram(prog);

	


	ILuint ilTexID[3]; /* ILuint is a 32bit unsigned integer.
    //Variable texid will be used to store image name. */

	ilInit(); /* Initialization of OpenIL */
	ilGenImages(3, ilTexID); /* Generation of three image names for OpenIL image loading */
	glGenTextures(3, texName); //and we eventually want the data in an OpenGL texture
 


	ilBindImage(ilTexID[0]); /* Binding of IL image name */
	loadTexFile("images/fire4.png");
	glBindTexture(GL_TEXTURE_2D, texName[0]); //bind OpenGL texture name


	/*ILubyte  firepixels;

	ILubyte * firepix = ilGetData();

	int size = ilGetInteger(IL_IMAGE_WIDTH) * ilGetInteger(IL_IMAGE_HEIGHT);

	for ( int i = 0; i < size; i++)
	{
		byte bix = firepix[i];

		byte newbix = (byte)PerlinNoise1D((double)bix, 1,1,5);

		int r = 0;
	}*/

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   //Note how we depend on OpenIL to supply information about the file we just loaded in
   glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),0,
	   ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_TYPE), ilGetData());

	//Now repeat the process for the second image
	ilBindImage(ilTexID[1]);
	
	glBindTexture(GL_TEXTURE_2D, texName[1]);
	loadTexFile("images/alpaca.png");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),0,
	ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_TYPE), ilGetData());


	//And the third image

	ilBindImage(ilTexID[2]);
	glBindTexture(GL_TEXTURE_2D, texName[2]);
	loadTexFile("images/opengl.png");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),0,
	ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_TYPE), ilGetData());

    ilDeleteImages(3, ilTexID); //we're done with OpenIL, so free up the memory

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	model_view = glGetUniformLocation(prog, "model_view");
	projection = glGetUniformLocation(prog, "projection");
	
	texMap = glGetUniformLocation(prog, "permTexture");
	glUniform1i(texMap, 0);//assign this one to texture unit 0


	glBindBuffer( GL_ARRAY_BUFFER, vbo[0] );
	vPosition = glGetAttribLocation(prog, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer( GL_ARRAY_BUFFER, vbo[1] );
	texCoord = glGetAttribLocation(prog, "texCoord");
	glEnableVertexAttribArray(texCoord);
	glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	 
	float time = 0.0f;
	  	  
	// Update the uniform time variable.
	location_time = glGetUniformLocation( prog, "utime" );
	location_Stability = glGetUniformLocation( prog, "uStability" );
}
void init(void)
{    
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glDisable(GL_DEPTH_TEST);

   // set Stability to 0
   Stability = 0.0;

   
   squareverts[0] = vec4(-1, -1, 0, 1);
   texcoords[0] = vec2(0, 0);
   squareverts[1] = vec4(1, -1, 0, 1);
   texcoords[1] = vec2(1, 0);
   squareverts[2] = vec4(1, 1, 0, 1);
   texcoords[2] = vec2(1, 1);
   squareverts[3] = vec4(1, 1, 0, 1);
   texcoords[3] = vec2(1, 1);
   squareverts[4] = vec4(-1, 1, 0, 1);
   texcoords[4] = vec2(0, 1);
   squareverts[5] = vec4(-1, -1, 0, 1);
   texcoords[5] = vec2(0, 0);

    // Create a vertex array object
    glGenVertexArrays( 1, &vao[0] );

    // Create and initialize any buffer objects
	glBindVertexArray( vao[0] );
	glGenBuffers( 2, &vbo[0] );
    glBindBuffer( GL_ARRAY_BUFFER, vbo[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(squareverts), squareverts, GL_STATIC_DRAW);

	glBindBuffer( GL_ARRAY_BUFFER, vbo[1] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);



   //program = InitShader( "vshader-texture.glsl", "fshader-texture.glsl" );
   // program = InitShader( "snoise3.vert", "snoise3.frag" ); // O
   programsimplex = InitShader( "fbmnoise.vert", "fbmnoise.frag" );
   programclassic = InitShader( "fbmnoise.vert", "classicnoise3d.frag" );
  
   sprintf(noisetypestring, "%s", "Classic 3D Noise"); 
   programObj = programclassic;
   setupShader(programObj, vao, vbo);

	
}

void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
	 glUseProgram(programObj);
	 
	 model_view = glGetUniformLocation(programObj, "model_view");
	 projection = glGetUniformLocation(programObj, "projection");

	  // glUniform1f() is bugged in Linux Nvidia driver 260.19.06,
	  // so we use glUniform1fv() instead to work around the bug.
	  /*if ( location_time != -1 ) {
		float time = (float)glutGet(GLUT_ELAPSED_TIME);
		glUniform1fv( location_time, 1, &time );
	  }*/

    static char titlestring[200];
	sprintf(titlestring, "GLSL Procedural Fire - %s - Stability (%.1f)", noisetypestring, Stability);
        

    glutSetWindowTitle(titlestring);

	glUniform1fv( location_Stability, 1, &Stability );

    mat4 camera = mv =  LookAt(vec4(0,0,5.0+z_distance,1),vec4(0,0,0,1),vec4(0,1,0,0))* RotateX(view_rotx) * RotateY(view_roty) * RotateZ(view_rotz);

	/////////////////////////////////////////////////////////////////////
	mv = mv*Scale(4,4,4);

	stack.push(mv);

	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv*Translate(0,0,0));
	glUniformMatrix4fv(projection, 1, GL_TRUE, p);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texName[0]); //which texture do we want?
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	
	mv = stack.pop();

	/////////////////////////////////////////////////////////////////
	//stack.push(mv);
	//glUniformMatrix4fv(model_view, 1, GL_TRUE, mv*Translate(1.1,0,0));
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, texName[1]); //which texture do we want?
	//glDrawArrays( GL_TRIANGLES, 0, 6 );
	//
	//mv = stack.pop();
	
	

	
   glutSwapBuffers();


}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

	p = Perspective(60.0, (float)w/(float)h, 1.0, 30.0);
	glUniformMatrix4fv(projection, 1, GL_TRUE, p);
}

void keyboard (unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
         break;
	  case GLUT_KEY_UP:
		  Stability += 0.1;
		  glutPostRedisplay();
		  break;
      case GLUT_KEY_DOWN:
		  Stability -= 0.1;
	  case 'c':
		  sprintf(noisetypestring, "%s", "Classic 3D Noise");
		  programObj = programclassic;
		  glUseProgram(programObj);
		  setupShader(programObj, vao, vbo);
		  break;
	  case 's':
		  sprintf(noisetypestring, "%s", "Simplex 3D Noise");
		  programObj = programsimplex;
		  glUseProgram(programObj);
		  setupShader(programObj, vao, vbo);
		  break;
      default:
         break;
   }
}
void specialFunc (int key, int x, int y)
{
   switch (key) {
      case GLUT_KEY_UP:
		  Stability += 0.1;
		  glutPostRedisplay();
		  break;
      case GLUT_KEY_DOWN:
		  Stability -= 0.1;
		  glutPostRedisplay();
		  break;
      default:
         break;
   }
}

void mouse(int button, int state, int x, int y) {
  //establish point of reference for dragging mouse in window
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
      left_button_down = TRUE;
	  prevMouseX= x;
      prevMouseY = y;
    }

	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
      right_button_down = TRUE;
      prevMouseX = x;
      prevMouseY = y;
    }
    else if (state == GLUT_UP) {
      left_button_down = FALSE;
	  right_button_down = FALSE;
	}
}
void myIdle()
{
	if ( location_time != -1 ) {
		float time = (float)glutGet(GLUT_ELAPSED_TIME);

		if ( ((int)time % 100) == 0 )
		{
			glUniform1fv( location_time, 1, &time );
			glutPostRedisplay();
		}
	  }
}
int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowSize(WIDTH, HEIGHT);
   glutInitWindowPosition(100, 100);
   glutCreateWindow("Fire");
   glewExperimental = GL_TRUE;

	glewInit();
   init();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(specialFunc);
   glutIdleFunc(myIdle);
   glutMouseFunc(mouse);
   
	
   glutMainLoop();
   return 0; 
}