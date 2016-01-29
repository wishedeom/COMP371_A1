#include "glew.h"		// include GL Extension Wrangler

#include "glfw3.h"  // include GLFW helper library

#include "glm.hpp"					// vector types
#include "gtc/matrix_transform.hpp"	// matrix types
#include "gtc/type_ptr.hpp"
#include "gtc/constants.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype>

//using namespace std;

#define M_PI        3.14159265358979323846264338327950288f   /* pi */
#define DEG_TO_RAD	M_PI/180.0f
#define INPUT_FILE	"input_a1.txt"

GLFWwindow* window = 0x00;

GLuint shader_program = 0;

GLuint view_matrix_id = 0;
GLuint model_matrix_id = 0;
GLuint proj_matrix_id = 0;


// Constant vectors
const glm::vec3 center		(0.0f, 0.0f,  0.0f);
const glm::vec3 up			(0.0f, 1.0f,  0.0f);
const glm::vec3 left		(1.0f, 0.0f,  0.0f);
const glm::vec3 initialEye	(0.0f, 0.0f, -1.0f);

// Screen size
int width  = 800;
int height = 800;

// Eye
glm::vec3 eye = initialEye;

///Transformations
glm::mat4 proj_matrix = glm::perspective(45.0f, (float) height / width, 0.1f, 10000.0f);
glm::mat4 view_matrix(1.0);
glm::mat4 model_matrix;

// Vertex buffer data
GLfloat* vertexBufferData;

GLuint VBO, VAO, EBO;

GLfloat point_size = 3.0f;

// An array of 3 vectors which represents 3 vertices
const GLfloat triangle_vertex_buffer_data[] =
{
	-0.5f,	-0.5f,	0.0f,
	-0.5f,	 0.5f,	0.0f,
	 0.5f,	-0.5f,	0.0f,
	 0.5f,	 0.5f,	0.0f
};

const GLuint indices[] =
{
	0, 2, 3,
	0, 3, 1
};

/// Handle the keyboard input
void keyPressed(GLFWwindow *_window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		break;
	case GLFW_KEY_LEFT:
		model_matrix = glm::rotate(model_matrix, glm::radians(10.0f), up);
		break;
	case GLFW_KEY_RIGHT:
		model_matrix = glm::rotate(model_matrix, glm::radians(-10.0f), up);
		break;
	case GLFW_KEY_UP:
		model_matrix = glm::rotate(model_matrix, glm::radians(10.0f), left);
		break;
	case GLFW_KEY_DOWN:
		model_matrix = glm::rotate(model_matrix, glm::radians(-10.0f), left);
		break;
	default:
		break;
	}
}

/// Handle mouse input

bool mouseButtonLeftDown = false;
double mousePosY = 0;
double mousePosY_down = 0;
glm::vec3 eyeMouseDown;

void cursorPositionCallback(GLFWwindow* _window, double xpos, double ypos)
{
	mousePosY = ypos;

	if (mouseButtonLeftDown)
	{
		eye = (float) ((mousePosY - mousePosY_down) / (height / 2) + 1) * eyeMouseDown;
	}
}


void mouseButtonCallback(GLFWwindow* _window, int button, int action, int mods)
{
	switch (button)
	{
		case GLFW_MOUSE_BUTTON_LEFT:
			if (action == GLFW_PRESS)
			{
				mouseButtonLeftDown = true;
				mousePosY_down = mousePosY;
				eyeMouseDown = eye;
			}
			else if (action == GLFW_RELEASE)
			{
				mouseButtonLeftDown = false;
			}
	}
}

// Handle window resizing
void windowSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	proj_matrix = glm::perspective(45.0f, 1.0f, 0.1f, 10000.0f);
}


glm::vec3 readVertex(std::ifstream* const file)
{
	int x, y, z;
	*file >> x;
	*file >> y;
	*file >> z;
	return glm::vec3(x, y, z);
}


// Given a file and a number of lines to read, returns a polyline build from given vertices.
std::vector<glm::vec3> readPolyline(std::ifstream* const file, const int numLines)
{
	std::vector<glm::vec3> polyline;
	for (int i = 1; i <= numLines; i++)
	{
		polyline.push_back(readVertex(file));
	}
	return polyline;
}

// The translational sweep is made from a profile polyline with p vertices and a trajectory polyline with t vertices, for a total of p * t vertices. The sweep itself
// consists of (p - 1) * (t - 1) recangles, paramaterized from 0 to p - 2 along the profile curve and from 0 to t - 2 along the trajectory curve. Since the
// vertices are numbered starting from 0 and increasing along the profile curve, then the trajectory curve, the vertex indices for the four vertices of rectange (i, j) are
// as follows.
//
//		 upper left (UL)		  upper right (UR)
//		(i + (j + 1) * p)		(i + (j + 1) * p + 1)
//				*-----------------------*
//				|                   /	|
//				|                /		|
//				|             /			|
//				|          /			|
//				|        /				|
//				|     /					|
//				|  /					|
//				*-----------------------*
//			(i + j * p)			(i + p * j + 1)
//		  lower left (LL)		lower right (LR)
//
// The vertices for the triangles to draw are
//
//		LL -> LR -> UR
//			then
//		LL -> UR -> UL
//
// This is done for each rectangle of the translational sweep mesh.

// Given a number of profile polyline vertices p and a number of trajectory polyline vertices t, computes the array of vertex indices to draw each triangle in the
// translational sweep.
GLuint* computeSweepIndices(const int p, const int t)
{
	std::vector<GLuint> indices;
	for (int i = 0; i < p - 1; i++)
	{
		for (int j = 0; j < t - 1; j++)
		{
			// Vertices of rectangle (i, j)
			const GLuint lowerLeft	= i + j	* p;
			const GLuint lowerRight	= i + j	* p + 1;
			const GLuint upperLeft	= i + (j + 1) * p;
			const GLuint upperRight	= i + (j + 1) * p + 1;

			// Lower triangle
			indices.push_back(lowerLeft);
			indices.push_back(lowerRight);
			indices.push_back(upperRight);

			// Upper triangle
			indices.push_back(lowerLeft);
			indices.push_back(upperRight);
			indices.push_back(upperLeft);
		}
	}
	return indices.data();
}


// Given a std::vector of 3D vectors representing vertices of a polyline (curve), computes the displacement of each vertex from the first vertex.
std::vector<glm::vec3> computeDisplacements(const std::vector<glm::vec3> polyline)
{

	std::vector<glm::vec3> displacements;	// To hold displacement vectors
	const auto firstVertex = polyline[0];	// First vertex in polyline
	for (auto vertex : polyline)
	{
		displacements.push_back(vertex - firstVertex);	// Compute the displacement from the first vertex to each vertex of the polyline,
														//and push that onto the vector of displacements.
	}
	return displacements;
}


// Compute the translational sweep generated by two polylines in 3D. The profileCurve is translated by the trajectoryCurve.
std::vector<glm::vec3> computeTranslationalSweep(const std::vector<glm::vec3> profilePolyline, const std::vector<glm::vec3> trajectoryPolyline)
{
	const auto trajectoryCurveDisplacements = computeDisplacements(trajectoryPolyline);	// Compute trajectory curve displacements.
	static std::vector<glm::vec3> translationalSweep;	// To hold translational sweep vertices.
	for (auto displacement : trajectoryCurveDisplacements)
	{
		for (auto profileVertex : profilePolyline)
		{
			// Iterate through the trajectory curve displacement vectors and translate each profile polyline vertex.
			translationalSweep.push_back(profileVertex + displacement);
		}
	}
	return translationalSweep;
}


// Given an input filename, opens the file and reads sweep vertex data from it.
std::vector<glm::vec3> readSweep(const std::string filename)
{
	std::ifstream vertexDataFile(filename); // Open input file
	std::vector<glm::vec3> sweep;

	if (vertexDataFile.is_open())
	{
		int numSpans;
		int numProfilePolylineVertices;
		int numTrajectoryPolylineVertices;

		vertexDataFile >> numSpans;						// Read number of spans: 0 for translational sweep, positive for rotational sweep

		vertexDataFile >> numProfilePolylineVertices;	// Read number of points in the profile polyline
		auto profilePolyline = readPolyline(&vertexDataFile, numProfilePolylineVertices);	// Read profile polyline vertex data

		// For translational sweep
		if (numSpans == 0)
		{
			vertexDataFile >> numTrajectoryPolylineVertices;											// Read number of vertices in the trajectory polyline
			auto trajectoryPolyline = readPolyline(&vertexDataFile, numTrajectoryPolylineVertices);		// Read trajectory polyline vertex data
			sweep = computeTranslationalSweep(profilePolyline, trajectoryPolyline);
		}
		else
		{
			// TO-DO:
			// return rotational sweep
		}

		vertexDataFile.close();
	}
	else
	{
		std::cout << "Unable to open file.";
	}

	return sweep;
}


GLfloat* getCoordinateArray(const std::vector<glm::vec3> sweep)
{
	std::vector<GLfloat> sweepCoordinates;
	for (auto vertex : sweep)
	{
		sweepCoordinates.push_back(vertex.x);
		sweepCoordinates.push_back(vertex.y);
		sweepCoordinates.push_back(vertex.z);
	}
	return sweepCoordinates.data();
}


bool initialize()
{
	/// Initialize GL context and O/S window using the GLFW helper library
	if (!glfwInit())
	{
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	/// Create a window of size 800x800 and with title "Lecture 2: First Triangle"
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	window = glfwCreateWindow(width, height, "COMP371: Assignment 1", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwGetWindowSize(window, &width, &height);
	///Register the keyboard callback function: keyPressed(...)
	glfwSetKeyCallback(window, keyPressed);

	///Regster the mouse cursor position callback function: cursorPositionCallback(...)
	glfwSetCursorPosCallback(window, cursorPositionCallback);

	///Regster the mouse button callback function: mouseButtonCallback(...)
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	///Register window resize callback function
	glfwSetWindowSizeCallback(window, windowSizeCallback);

	glfwMakeContextCurrent(window);

	/// Initialize GLEW extension handler
	glewExperimental = GL_TRUE;	///Needed to get the latest version of OpenGL
	glewInit();

	/// Get the current OpenGL version
	const GLubyte* renderer = glGetString(GL_RENDERER); /// Get renderer string
	const GLubyte* version = glGetString(GL_VERSION); /// Version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	/// Enable the depth test i.e. draw a pixel if it's closer to the viewer
	glEnable(GL_DEPTH_TEST); /// Enable depth-testing
	glDepthFunc(GL_LESS);	/// The type of testing i.e. a smaller value as "closer"

	return true;
}

bool cleanUp()
{
	glDisableVertexAttribArray(0);
	//Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// Close GL context and any other GLFW resources
	glfwTerminate();

	return true;
}

GLuint loadShaders(std::string vertex_shader_path, std::string fragment_shader_path)
{
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_shader_path, std::ios::in);
	if (VertexShaderStream.is_open())
	{
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else
	{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_shader_path.c_str());
		getchar();
		exit(-1);
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_shader_path, std::ios::in);
	if (FragmentShaderStream.is_open())
	{
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_shader_path.c_str());
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_shader_path.c_str());
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);

	glBindAttribLocation(ProgramID, 0, "in_Position");

	//appearing in the vertex shader.
	glBindAttribLocation(ProgramID, 1, "in_Color");

	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	//The three variables below hold the id of each of the variables in the shader
	//If you read the vertex shader file you'll see that the same variable names are used.
	view_matrix_id = glGetUniformLocation(ProgramID, "view_matrix");
	model_matrix_id = glGetUniformLocation(ProgramID, "model_matrix");
	proj_matrix_id = glGetUniformLocation(ProgramID, "proj_matrix");

	return ProgramID;
}


int main()
{
	auto sweep = readSweep(INPUT_FILE);				// Compute sweep vertices
	vertexBufferData = getCoordinateArray(sweep);	// Put sweep vertices into array

	// Test
	for (int i = 0; i < (sizeof(vertexBufferData) / sizeof(*vertexBufferData)); i++)
	{
		std::cout << vertexBufferData[i] << std::endl;
	}
	// End test

	initialize();

	///Load the shaders
	shader_program = loadShaders("COMP371_hw1.vs", "COMP371_hw1.fs");

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertex_buffer_data), triangle_vertex_buffer_data, GL_STATIC_DRAW);

	//
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	//

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,						// attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,						// size
		GL_FLOAT,				// type
		GL_FALSE,				// normalized?
		3 * sizeof(GLuint),		// stride
		(GLvoid*) 0				// array buffer offset
		);



	while (!glfwWindowShouldClose(window))
	{
		view_matrix = glm::lookAt(eye, center, up);

		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
		glPointSize(point_size);

		glUseProgram(shader_program);

		//Pass the values of the three matrices to the shaders
		glUniformMatrix4fv(proj_matrix_id, 1, GL_FALSE, glm::value_ptr(proj_matrix));
		glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(model_matrix_id, 1, GL_FALSE, glm::value_ptr(model_matrix));

		glBindVertexArray(VAO);
		// Draw the triangle !
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		// update other events like input handling
		glfwPollEvents();
		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(window);
	}

	cleanUp();
	return 0;
}