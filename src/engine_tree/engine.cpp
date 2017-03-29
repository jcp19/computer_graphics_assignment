//#define TIXML_USE_STL 
#include <vector>
#include <iostream>
#include "tinyxml2.h"
#include "point.h"
#include "triangle.h"
#include <fstream>
#include <string>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include "group.h"
#include <ctype.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "transform.h"
#include "scale.h"
#include "translate.h"
#include "rotate.h"

using namespace tinyxml2;
using namespace std;

// TODO
// Por cores

// Camera control
float r = 10.0f;
float alpha;
float beta;

// Polygon Mode
GLenum mode;

// Structure to save figures to draw
//vector<figure> figures;

vector<group> groups;
map<string, figure> models;

// directory of the read file
string directory; 

// need to correct to accept full paths
string directory_of_file(const string& fname) {
	size_t pos = fname.find_last_of("\\/");

	return (string::npos == pos)? "" : fname.substr(0, pos+1);
}

float randFloat() {
	 return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

void changeSize(int w, int h) {
	if(h == 0)
		h = 1;
	float ratio = w * 1.0 / h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(45.0f ,ratio, 1.0f ,1000.0f);
	glMatrixMode(GL_MODELVIEW);
}

void drawTriangle(triangle t){
	glColor3f(t.color_r, t.color_g, t.color_b);
	glVertex3f(t.p1.x, t.p1.y, t.p1.z);
	glVertex3f(t.p2.x, t.p2.y, t.p2.z);
	glVertex3f(t.p3.x, t.p3.y, t.p3.z);
}

void print_matrix(float m[], int I, int J){
	for(int i  = 0; i < I; i++){
		for(int j  = 0; j < J; j++){
			cout << m[i + j*4] << " ";
		}
		cout << endl;
	}
}

void drawGroup(group g){
	glPushMatrix();
	for(transform* t : g.transforms){
		t->apply();
	}
	for(string model: g.models){
		glBegin(GL_TRIANGLES);
		figure f = models[model];
		for(triangle t:f){
			drawTriangle(t);
		}
		glEnd();
	}
	for(auto gr : g.child_groups){
		drawGroup(gr);
	}
	glPopMatrix();
}

void renderScene(void) {
        GLenum modes[] = {GL_FILL, GL_LINE, GL_POINT};
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT, modes[mode]);
	glLoadIdentity();
	gluLookAt(r*cosf(beta)*cosf(alpha), r*sinf(beta), r*cosf(beta)*sinf(alpha), 
		      0.0,0.0,0.0,
			  0.0f,1.0f,0.0f);

	for(auto g : groups){
		drawGroup(g);
		/*glMultMatrixf(g.referential);
		//cout << "=============================================" << endl;
		glBegin(GL_TRIANGLES);
		for(auto f : g.models){
			for(triangle t:f){
				drawTriangle(t);
			}
		}
		glEnd();*/
		//print_matrix(g.referential, 4, 4);
	}
	// End of frame
	glutSwapBuffers();
}

void processKeys(unsigned char c, int xx, int yy) {
// put code to process regular keys in here
	switch(toupper(c)){
		case 'M': // More radius
			r += 0.2f;
			break;
		case 'L': // Less radius
			r -= 0.2f;
			if(r < 0.2f)
				r = 0.2f;
			break;
		case 'C':
			mode = (mode + 1) % 3;
			break;
	}
	glutPostRedisplay();
}

void processSpecialKeys(int key, int xx, int yy) {
// put code to process special keys in here
	switch(key){
		case GLUT_KEY_UP:
			beta += 0.1f;
			if(beta > 1.5f)
				beta = 1.5f;
			break;
		case GLUT_KEY_LEFT:
			alpha += 0.1f;
			break;
		case GLUT_KEY_DOWN:
			beta -= 0.1f;
			if(beta < -1.5f)
				beta = -1.5;
			break;
		case GLUT_KEY_RIGHT:
			alpha -= 0.1f;
			break;
	}
	glutPostRedisplay();
}

group parseGroup(XMLElement* gr) {
	group g;
	XMLElement * child = gr->FirstChildElement();
	for( ; child; child = child->NextSiblingElement()) {
		string type = string(child->Name()); 
		cout << "tipo: " << type << endl;
		if(type == "translate"){
			float x, y, z;
			int rX, rY,rZ;

			rX = child->QueryFloatAttribute("X", &x);
			rY = child->QueryFloatAttribute("Y", &y);
			rZ = child->QueryFloatAttribute("Z", &z);

			/* probably not needed, if QueryIntAttribute does not change the value of the var when
			 * the attribute is not present
			 */
			// MUDAR TIPO rX
			x = (rX == XML_SUCCESS)? x : 0;
			y = (rY == XML_SUCCESS)? y : 0;
			z = (rZ == XML_SUCCESS)? z : 0;

			g.transforms.push_back(new translate(x,y,z));
			cout << "translation read " << x << " " << y << " " << z << "\n";
		} else if(type == "rotate"){
			float angle, axisX, axisY, axisZ;
			int rAngle, rAxisX, rAxisY, rAxisZ;

			rAngle = child->QueryFloatAttribute("angle", &angle);
			rAxisX = child->QueryFloatAttribute("axisX", &axisX);
			rAxisY = child->QueryFloatAttribute("axisY", &axisY);
			rAxisZ = child->QueryFloatAttribute("axisZ", &axisZ);

			axisX = (rAxisX == XML_SUCCESS)? axisX : 0.0;
			axisY = (rAxisY == XML_SUCCESS)? axisY : 0.0;
			axisZ = (rAxisZ == XML_SUCCESS)? axisZ : 0.0;
			angle = (rAngle == XML_SUCCESS)? angle : 0.0;

			g.transforms.push_back(new rotate(angle, axisX, axisY, axisZ));
			cout << "rotation read " << angle << " " << axisX << " " << axisY << " " << axisZ << "\n";
		} else if(type == "scale"){
			float x, y, z;
			int rX, rY,rZ;

			rX = child->QueryFloatAttribute("X", &x);
			rY = child->QueryFloatAttribute("Y", &y);
			rZ = child->QueryFloatAttribute("Z", &z);

			x = (rX == XML_SUCCESS)? x : 1;
			y = (rY == XML_SUCCESS)? y : 1;
			z = (rZ == XML_SUCCESS)? z : 1;
			//glScalef(x, y, z);
			g.transforms.push_back(new scale(x,y,z));
			cout << "scale read: " << x << " " << y <<" " << z <<"\n";
		} else if(type == "group"){
			group g_child = parseGroup(child);
			g.child_groups.push_back(g_child);
			cout << "group read\n";
		} else if(type == "models"){
			cout << "models read\n";
			XMLElement* model = child->FirstChildElement("model");
			for(; model; model=model->NextSiblingElement()){
				const char * filename= model->Attribute("file");
				// cout << "ficheiro lido: " << filename << endl;

				if(filename != NULL) {
					string f_name = string(filename);
					int n_vertex, n_triangles;

					ifstream file(directory + filename);

					if(!file) {
						cerr << "The file \"" << filename << "\" was not found.\n";
					}
					// estrutura com cores deve ficar aqui
					g.models.push_back(f_name);
					if(models.find(f_name) != models.end()){
						// if the model file was already read
						continue;
					}

					file >> n_vertex; // reads the number of vertices from the file
					n_triangles = n_vertex/3;
					figure triangles;

					for(int i = 0; i < n_triangles; i++){
						float color_r, color_g, color_b;
						point ps[3];
						for(int j = 0; j < 3; j++){
							float px, py, pz;
							file >> px;
							file >> py;
							file >> pz;
							point p(px, py, pz);
							ps[j] = p;
						}
						color_r = randFloat();
						color_g = randFloat();
						color_b = randFloat();

						triangle t(ps[0], ps[1], ps[2], color_r, color_g, color_b);
						triangles.push_back(t);
					}
					file.close();
					//inserir map da string para a figura
					//g.figures.push_back(triangles);
					//figures.push_back(triangles);
					models[f_name] = triangles;
				}
			}
			// guardar matriz!
			//glGetFloatv (GL_MODELVIEW_MATRIX, g.referential); // save matrix for later
		}
	}
	glPopMatrix();
	return g;
}

//We assume that the .xml and .3d files passed are correct.
int main(int argc, char** argv){
	if(argc != 2){
		cerr << "Usage: " << argv[0] << " config_file\n";
		return 1;
	}

	XMLDocument doc;
	XMLError loadOkay = doc.LoadFile(argv[1]);

	if(loadOkay != XML_SUCCESS){ // Condicao para erro
		perror("");
		cerr << "Error loading file '" << argv[1] << "'.\n";
		return 1;
	}

	directory = directory_of_file(argv[1]);
	XMLElement* gr = doc.FirstChildElement("scene")->FirstChildElement("group");
	for(; gr; gr=gr->NextSiblingElement()){
		group g = parseGroup(gr);
		groups.push_back(g);
	}

	// init GLUT and the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(800,800);
	glutCreateWindow("Pratical Assignment");

	// Required callback registry 
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processKeys);
	glutSpecialFunc(processSpecialKeys);

	//  OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_CULL_FACE);

	glutMainLoop();

	return 1;
}