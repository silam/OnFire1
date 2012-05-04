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

#define WIDTH 1000
#define HEIGHT 400
#define TRUE 1
#define FALSE 0

matrix_stack stack;

GLint location_time = -1;
GLint location_Stability = -1;
GLint location_Roughness = -1;

static char noisetypestring[200];
vec4 squareverts[6];
vec2 texcoords[6];

float time;




double view_rotx = 180.0;
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
int Roughness;

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
	location_Roughness = glGetUniformLocation( prog, "uRoughness" );
}
void init(void)
{    
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glDisable(GL_DEPTH_TEST);

   // set Stability to 0
   Stability = 0.0;
   Roughness = 0;
   
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

	
  
   programsimplex = InitShader( "noise3d.vert", "simplexnoise3d.frag" );
   programclassic = InitShader( "noise3d.vert", "classicnoise3d.frag" );
  
   sprintf(noisetypestring, "%s", "Classic 3D Noise"); 
   programObj = programclassic;
   setupShader(programObj, vao, vbo);

	
}

///////////////////////////////
// Display function
///////////////////////////////
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
	// printing window title
    static char titlestring[200];
	sprintf(titlestring, "GLSL Procedural Fire - Classic vs Simplex 3D Noise - Stability (%.1f) - Roughnes Level (%d)", Stability, Roughness);
	glutSetWindowTitle(titlestring);

	mat4 camera = mv =  LookAt(vec4(0,0,5.0+z_distance,1),vec4(0,0,0,1),vec4(0,1,0,0))* RotateX(view_rotx);
		
	mv = mv*Scale(4,4,4);


	/////////////////////////////////////////////////////////////////
	// Rendering texture with Classic noise
	/////////////////////////////////////////////////////////////////
	glUseProgram(programclassic);
	 
	model_view = glGetUniformLocation(programclassic, "model_view");
	projection = glGetUniformLocation(programclassic, "projection");
	location_time = glGetUniformLocation( programclassic, "utime" );
	location_Stability = glGetUniformLocation( programclassic, "uStability" );
	location_Roughness = glGetUniformLocation( programclassic, "uRoughness" );

	

	// set stability value
	glUniform1fv( location_Stability, 1, &Stability );
	glUniform1iv( location_Roughness, 1, &Roughness );
	glUniform1fv( location_time, 1, &time );

    
	stack.push(mv);

	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv*Translate(-0.4,0,0));
	glUniformMatrix4fv(projection, 1, GL_TRUE, p);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texName[0]); //which texture do we want?
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	
	mv = stack.pop();

	/////////////////////////////////////////////////////////////////
	// Rendering texture with simplex noise
	/////////////////////////////////////////////////////////////////
	stack.push(mv);

	glUseProgram(programsimplex);
	 
	model_view = glGetUniformLocation(programsimplex, "model_view");
	projection = glGetUniformLocation(programsimplex, "projection");
	location_time = glGetUniformLocation( programsimplex, "utime" );
	location_Stability = glGetUniformLocation( programsimplex, "uStability" );
	location_Roughness = glGetUniformLocation( programsimplex, "uRoughness" );

	glUniform1fv( location_time, 1, &time );
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv*Translate(1.1,0,0));
	glUniformMatrix4fv(projection, 1, GL_TRUE, p);
	// set stability value
	glUniform1fv( location_Stability, 1, &Stability );
	glUniform1iv( location_Roughness, 1, &Roughness );



	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texName[0]); //which texture do we want?
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	mv = stack.pop();
	/////////////////////////////////////////////////////////////////

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
		  glutPostRedisplay();
		  break;
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
	  case '+':
		  Roughness += 1;
		  glutPostRedisplay();
		  break;
	  case '-':

		  if ( Roughness > 0 )
			Roughness -= 1;
		  
		  glutPostRedisplay();
		  break;
      default:
         break;
   }
}

//////////////////////////////////////////////
// Special key func to increase/decrease stability of the fire
//////////////////////////////////////////////
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
//////////////////////////////////////
// Idle function to change the time
/////////////////////////////////////
void myIdle()
{
	if ( location_time != -1 ) {
		time = (float)glutGet(GLUT_ELAPSED_TIME);

		if ( ((int)time % 100) == 0 )
		{

			//glUseProgram(programObj);
			//location_time = glGetUniformLocation( programObj, "utime" );

			//glUniform1fv( location_time, 1, &time );
			glutPostRedisplay();
		}
	  }
}
//////////////////////////////////////
// Main
/////////////////////////////////////
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
  
   glutMainLoop();
   return 0; 
}