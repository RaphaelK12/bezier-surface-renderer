#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <sstream>
#include <iterator>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>

#include "bezier.h"
#include "utility.h"

#ifdef _WIN32
static DWORD lastTime;
#else
static struct timeval lastTime;
#endif

using namespace std;
#define PI 3.14159265

//****************************************************
// Global Variables
//****************************************************

//Change in location when keys are pressed
float degree_step = 1;
float translation_step = 0.4;

string input_file_name;
string output_file_name = "";
float sub_div_parameter;
// switch from uniform to adaptive mode 
bool adaptive = false;
bool flat_shading = true;
bool wireframe = false;
bool hiddenLineMode = false;
Model model;

float zoom = 15.0f;
float rotx = 0;
float roty = 0.001f;
float tx = 0;
float ty = 0;
int lastx=0;
int lasty=0;
unsigned char Buttons[3] = {0};

// light color
float r = 1.0;
float b = 0.0;
float g = 0.0;

//****************************************************
// Some Classes
//****************************************************

class Viewport {
  public:
    int w, h; // width and height
};
Viewport viewport;

//****************************************************
// Some Functions
//****************************************************

/******* lighting **********/
void light (void) {
    
    GLfloat color[] = {r, g, b};
    GLfloat light_position[] = {-7.0, -7.0, 7.0, 0.0};
    
    glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}
/******* lighting **********/


/*
Extracts commandline arguments, or returns an error if arguments are not valid.
Commandline arguments should be formatted like this:
main [inputfile.bez] [float subdivision parameter] optional[-a]

example:
./main models/teapot.bez 0.5
*/
void parseCommandlineArguments(int argc, char *argv[]) {
  if(argc < 3) {
    printf("\nWrong number of command-line arguments. Arguments should be in the format:\n");
    printf("main [inputfile.bez] [float subdivision parameter] optional[-a]\n\n");
    exit(0);
  }
  input_file_name = argv[1];
  sub_div_parameter = atof(argv[2]);
  // The optional parameters
  if(argc > 3) {
    int i = 3;
    while(i < argc) {      
      if(strcmp(argv[i],"-a") == 0) {
        adaptive = true;
      } else if(strcmp(argv[i],"-o") == 0) {
        output_file_name = argv[i+1];
        i++;
      } else if(strcmp(argv[i],"-color") == 0) {
        r = atof(argv[i+1]);
        g = atof(argv[i+2]);
        b = atof(argv[i+3]);
        i += 3;
      }
      i++;
    }
  }
}


/*
Splits a string at white space and returns extracted words as strings in a vector 
*/
vector<string> splitAtWhiteSpace(string const &input) { 
    istringstream buffer(input);
    vector<string> ret((istream_iterator<string>(buffer)), istream_iterator<string>());
    return ret;
}


/*
Parses input file and returns a model
*/
Model parseBezFile() {
  ifstream input_file(input_file_name.c_str());
  string line;
  if(input_file.is_open()) {
    int numPatches = 0;
    getline(input_file, line);
    numPatches = atoi(line.c_str());
    vector<Patch> patches;
    for (int i = 0; i < numPatches; i++){
        Point point_array[4][4];
        for(int c = 0; c < 4; c++) {
            getline(input_file, line);
            vector<string> coor_list;
            coor_list = splitAtWhiteSpace(line);
            
            for(int p = 0; p < 4; p++) {
                float x = atof(coor_list[3*p].c_str());
                float y = atof(coor_list[3*p+1].c_str());
                float z = atof(coor_list[3*p+2].c_str());
                Point point(x, y, z);
                point_array[c][p] = point;
            }
        }
        Patch patch(point_array);
        patches.push_back(patch);
        
        getline(input_file, line); //blank line between patches
    }
    input_file.close();
    Model model(patches, Color(0, 1, 0));
    return model;
  }
  printf("input file was not found\n");
  exit(1);
}


Model parseObjFile() {
  ifstream input_file(input_file_name.c_str());
  string line;
  
  vector<Patch> patches;
  Patch p;
  
  if(input_file.is_open()) {
      vector<Point> points;
      while (getline(input_file, line)) {
          vector<string> llist;
          llist = splitAtWhiteSpace(line);
          if (llist.size() > 0 && llist[0].compare("v") == 0) { //point
              Point p(atof(llist[1].c_str()), atof(llist[2].c_str()), atof(llist[3].c_str()));
              points.push_back(p);
          } else if(llist.size() > 0 && llist[0].compare("f") == 0) { //surface
              int point_index1 = atoi(llist[1].c_str());
              int point_index2 = atoi(llist[2].c_str());
              int point_index3 = atoi(llist[3].c_str());
              vector<Triangle> triangles;
              
              if(llist.size() == 5) {
                  int point_index4 = atoi(llist[4].c_str());
                  Triangle t1(points[point_index4-1], points[point_index1-1], points[point_index2-1]);
                  Triangle t2(points[point_index4-1], points[point_index2-1], points[point_index3-1]);
                  triangles.push_back(t1);
                  triangles.push_back(t2);
              } else if(llist.size() == 4) {
                  Triangle t1(points[point_index1-1], points[point_index2-1], points[point_index3-1]);
                  triangles.push_back(t1);
              } else {
                  printf("This model contains more than just triangle and quad representations!\n");
              }
              p.triangles = triangles;
              patches.push_back(p);
          }
      }
      Model m;
      m.patches = patches;
      return m;
  } else {
      printf("input file was not found\n");
      exit(1);
  }
}


void modelToOutputFile(Model model) {
  Patch curr_patch;
  Triangle curr_triangle;
  int vertex_count = 1;
  ofstream output_file;
  output_file.open (output_file_name);
  Point a,b,c;

  for(int i = 0; i < model.patches.size(); i++) {
    curr_patch = model.patches[i];
    for(int j = 0; j < curr_patch.triangles.size(); j++) {
      curr_triangle = curr_patch.triangles[j];
      a = curr_triangle.a;
      b = curr_triangle.b;
      c = curr_triangle.c;
      output_file << "v " << a.x << " " << a.y << " " << a.z << "\n";
      output_file << "v " << b.x << " " << b.y << " " << b.z << "\n";
      output_file << "v " << c.x << " " << c.y << " " << c.z << "\n";
      output_file << "f " << vertex_count << " " << vertex_count+1 << " " << vertex_count+2 << "\n";
      vertex_count += 3;
    }
  }
  output_file.close();
}

//****************************************************
// Keyboard functions
//****************************************************


void special_keyboard(int key, int x, int y){
  glMatrixMode(GL_MODELVIEW);
  
  switch(key){
      
      case GLUT_KEY_RIGHT:
        if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
          // object will be translated right
          tx += translation_step;
          break;
        }
        // object will be rotated right
        roty += degree_step;
        break;

      case GLUT_KEY_LEFT:
        if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
          // object will be translated left
          tx -= translation_step;
          break;
        }
        // object will be rotated left
        roty -= degree_step;
        break;

      case GLUT_KEY_UP:
        if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
          // object will be translated up
          ty += translation_step;
          break;
        }
        // object will be rotated up
        rotx -= degree_step;
        break;

      case GLUT_KEY_DOWN:
        if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
          // object will be translated down
          ty -= translation_step;
          break;
        }
        // object will be rotated down
        rotx += degree_step;
        break;

  }

}


void keyboard(unsigned char key, int x, int y){
  glMatrixMode(GL_MODELVIEW);
  switch(key){
      case 's':
        // toggle between flat and smooth shading
        flat_shading = !flat_shading;
        if (flat_shading){
        	glShadeModel(GL_FLAT);
        }else{
        	glShadeModel(GL_SMOOTH);
        }
        break;
    
      case 'w':
        // toggle between filled and wireframe mode
        wireframe = !wireframe;
        break;
        
	case 'h':
        // toggle between filled and wireframe mode
        hiddenLineMode = !hiddenLineMode;
        break;
    
      case 'c':
        // do vertex color shading based on the Gaussian Curvature of the surface.
        break;
    
      case 61: // = sign (PLUS)
        // zoom in
        zoom -= translation_step;
        break;
    
      case 45: // MINUS sign -
        // zoom out
        zoom += translation_step;
        break;
  }
}


//****************************************************
// Reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;
  
  glMatrixMode(GL_PROJECTION);  
  glViewport(0,0,viewport.w,viewport.h);// sets the rectangle that will be the window
  glLoadIdentity();                // loading the identity matrix for the screen

  
	gluPerspective(45,(float)w/h,0.1,100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


//****************************************************
// sets the window up
//****************************************************
void initScene(){
// prevent divide by 0 error when minimised
if(viewport.w==0) 
	viewport.h = 1;

glViewport(0,0,viewport.w,viewport.h);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
gluPerspective(45,(float)viewport.w/viewport.h,0.1,100);
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
  
  /******* lighting **********/
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  /******* lighting **********/
  
}


//***************************************************
// function that does the actual drawing
//***************************************************
void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glLoadIdentity();

	glTranslatef(0,0,-zoom);
	glTranslatef(tx,ty,0);
	glRotatef(rotx,1,0,0);
	glRotatef(roty,0,1,0);


  ///-----------------------------------------------------------------------
  light();
  //GLfloat purple[] = {1.0, 0, 1.0}; 
  //glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, purple);
  
  if(wireframe) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  	  glDisable(GL_LIGHTING);
      model.draw();
  }
  else if(hiddenLineMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_LIGHTING);
        model.draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.5, 0.5);
        glColor3f(0.0,0.0,0.0);
        model.draw();
        glDisable(GL_POLYGON_OFFSET_FILL);
        glColor3f(1.0,1.0,1.0);
    } else {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    	glEnable(GL_LIGHTING);
        if (flat_shading){
        	model.drawFlat();
        }else{
        	model.draw();
        }
    }  

  glFlush();
  glutSwapBuffers();                           // swap buffers (we earlier set double buffer)
}


//****************************************************
// called by glut when there are no messages to handle
//****************************************************
void myFrameMove() {
  //nothing here for now
#ifdef _WIN32
  Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
  glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}

void Motion(int x,int y)
{
	int diffx=x-lastx;
	int diffy=y-lasty;
	lastx=x;
	lasty=y;

	if( Buttons[0] && Buttons[1] )
	{
		zoom -= (float) 0.05f * diffx;
	}
	else
		if( Buttons[0] )
		{
			rotx += (float) 0.5f * diffy;
			roty += (float) 0.5f * diffx;		
		}
		else
			if( Buttons[1] )
			{
				tx += (float) 0.05f * diffx;
				ty -= (float) 0.05f * diffy;
			}
			glutPostRedisplay();
}


void Mouse(int b,int s,int x,int y)
{
	lastx=x;
	lasty=y;
	switch(b)
	{
	case GLUT_LEFT_BUTTON:
		Buttons[0] = ((GLUT_DOWN==s)?1:0);
		break;
	case GLUT_MIDDLE_BUTTON:
		Buttons[1] = ((GLUT_DOWN==s)?1:0);
		break;
	case GLUT_RIGHT_BUTTON:
		Buttons[2] = ((GLUT_DOWN==s)?1:0);
		break;
	default:
		break;		
	}
	glutPostRedisplay();
}


//****************************************************
// main
//****************************************************

int main(int argc, char *argv[]) {

  parseCommandlineArguments(argc, argv);
  
  string extension = input_file_name.substr(input_file_name.size() - 3);
  if(extension.compare("bez") == 0) {
      model = parseBezFile();
      if (adaptive){
          model.aSubDivide(sub_div_parameter);
      }else{
          model.uSubDivide(sub_div_parameter);
      }
  } else {
      model = parseObjFile();
  }
  
  if(output_file_name.compare("") != 0) {
    modelToOutputFile(model);
    exit(0);
  }
  
  //This initializes glut
  glutInit(&argc, argv);

  //This tells glut to use a double-buffered window with red, green, and blue channels 
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB|GLUT_DEPTH);

  // Initalize theviewport size
  viewport.w = 1000;//1280
  viewport.h = 1000;

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("CS184! by Sebastian and Risa");

  initScene();                                 // quick function to set up scene
  glutDisplayFunc(myDisplay);                  // function to run when its time to draw something
  glutReshapeFunc(myReshape);                  // function to run when the window gets resized
  
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special_keyboard);
  glutMouseFunc(Mouse);
  glutMotionFunc(Motion);

  glutIdleFunc(myFrameMove);                   // function to run when not handling any other task
  glutMainLoop();                              // infinite loop that will keep drawing and resizing and whatever else

  return 0;
}
