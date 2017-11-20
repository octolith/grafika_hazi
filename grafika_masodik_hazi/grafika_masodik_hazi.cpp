//=============================================================================================
// Framework for the ray tracing homework
// ---------------------------------------------------------------------------------------------
// Name   : 
// Neptun : 
//=============================================================================================

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

const unsigned int windowWidth = 600, windowHeight = 600;

int majorVersion = 3, minorVersion = 3;

#define MAXDEPTH 10
#define LIGHTS 0
#define INTERSECTABLES 0

struct vec3 {
	float x, y, z;

	vec3(float x0 = 0, float y0 = 0, float z0 = 0) {
		x = x0; y = y0; z = z0;
	}

	vec3 operator*(float a) const {
		return vec3(x * a, y * a, z * a);
	}
	vec3 operator/(float a) const {
		return vec3(x / a, y / a, z / a);
	}
	vec3 operator+(const vec3& v) const {
		return vec3(x + v.x, y + v.y, z + v.z);
	}
	vec3 operator-(const vec3& v) const {
		return vec3(x - v.x, y - v.y, z - v.z);
	}
	vec3 operator*(const vec3& v) const {
		return vec3(x * v.x, y * v.y, z * v.z);
	}
	vec3 operator-() const {
		return vec3(-x, -y, -z);
	}
	vec3 normalize() const {
		return (*this) * (1 / (Length() + 0.000001f));
	}
	float Length() const {
		return sqrtf(x * x + y * y + z * z);
	}

	operator float*() {
		return &x;
	}
};

float dot(const vec3& v1, const vec3& v2) {
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

vec3 cross(const vec3& v1, const vec3& v2) {
	return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

float sign(float f) {
	if (f < 0) {
		return -1.0f;
	}
	else {
		return 1.0f;
	}
}

struct mat4 {
public:
	float m[4][4];
	mat4() {}
	mat4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) {
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	mat4 operator*(const mat4& right) {
		mat4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
			}
		}
		return result;
	}
	operator float*() {
		return &m[0][0];
	}

	void SetUniform(unsigned shaderProg, char * name) {
		int loc = glGetUniformLocation(shaderProg, name);
		glUniformMatrix4fv(loc, 1, GL_TRUE, &m[0][0]);
	}
};

mat4 Translate(float tx, float ty, float tz) {
	return mat4(1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		tx, ty, tz, 1);
}

//mat4 Rotate(float angle, float wx, float wy, float wz) { … }
//mat4 Scale(float sx, float sy, float sz) { … }


class Material {
public:
	vec3 F0;
	vec3 n;		// toresmutato
	vec3 kd, ks;
	float shine;
	bool rough;
	bool reflective;
	bool refractive;
	vec3 ka;

	vec3 reflect(vec3 inDir, vec3 normal) {
		return inDir - normal * dot(normal, inDir) * 2.0f;
	}
	vec3 refract(vec3 inDir, vec3 normal) {

		// TODO színeket a saját törésmutatójuknak megfelelően
		float ior = n; // depends on whether from inside or outside
		float cosa = -dot(normal, inDir); // from inside?
		if (cosa < 0) {
			cosa = -cosa;
			normal = -normal;
			ior = 1 / n;
		}
		float disc = 1 - (1 - cosa * cosa) / ior / ior;
		if (disc < 0) return reflect(inDir, normal); // magic !!!
		return inDir / ior + normal * (cosa / ior - sqrt(disc));
	}
	vec3 Fresnel(vec3 inDir, vec3 normal) { // in/out irrelevant
		float cosa = fabs(dot(normal, inDir));

		// TODO: use regular Fresnel equation
		return F0 + (vec3(1, 1, 1) - F0) * pow(1 - cosa, 5);
	}
	vec3 shade(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 inRad) {
		vec3 reflRad(0, 0, 0);
		float cosTheta = dot(normal, lightDir);
		if (cosTheta < 0) return reflRad;
		reflRad = inRad * kd * cosTheta;;
		vec3 halfway = (viewDir + lightDir).normalize();
		float cosDelta = dot(normal, halfway);
		if (cosDelta < 0) return reflRad;
		return reflRad + inRad * ks * pow(cosDelta, shine);
	}
};

class GoldMaterial : public Material {
public:
	GoldMaterial() {
		rough = false;
		reflective = true;
		refractive = false;
		n = vec3(0.17f, 0.35f, 1.5f);
		ks = vec3(3.1f, 2.7f, 1.9f);
		ka = vec3(3.1f, 2.7f, 1.9f);
	}
};

class GlassMaterial : public Material {
public:
	GlassMaterial() {
		reflective = true;
		refractive = true;
		n = vec3(1.5f, 1.5f, 1.5f);
		ks = vec3(0.0f, 0.0f, 0.0f);
		ka = vec3(0.0f, 0.0f, 0.0f);
	}
};

class SilverMaterial : public Material {
public:
	SilverMaterial() {
		rough = false;
		reflective = true;
		refractive = false;
		n = vec3(0.14f, 0.16f, 0.13f);
		ks = vec3(4.1f, 2.3f, 3.1f);
	}
};

struct Ray {
	vec3 pos;
	vec3 dir;
	Ray(vec3 pos, vec3 dir) {
		this->pos = pos;
		this->dir = dir;
	}
};

struct Hit {
	float t;
	vec3 position;
	vec3 normal;
	Material* material;
	Hit() {
		t = -1;
	}
};

class Intersectable {
public:
	Material* material;
	virtual Hit intersect(const Ray& ray) = 0;
};

class Camera {
public:
	static const int XM = 600;
	static const int YM = 600;
	vec3 eye;
	vec3 lookat;
	vec3 up;
	vec3 right;
	float fov, asp, fp, bp;

	mat4 V() { // view matrix
		vec3 w = (eye - lookat).normalize();
		vec3 u = cross(up, w).normalize();
		vec3 v = cross(w, u);
		return Translate(-eye.x, -eye.y, -eye.z) *
			mat4(u.x, v.x, w.x, 0.0f,
				u.y, v.y, w.y, 0.0f,
				u.z, v.z, w.z, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
	}

	mat4 P() { // projection matrix
		float sy = 1 / tan(fov / 2);
		return mat4(sy / asp, 0.0f, 0.0f, 0.0f,
			0.0f, sy, 0.0f, 0.0f,
			0.0f, 0.0f, -(fp + bp) / (bp - fp), -1.0f,
			0.0f, 0.0f, -2 * fp*bp / (bp - fp), 0.0f);
	}

	Ray GetRay(float x, float y) {
		vec3 p = lookat + right * (2 * x / XM - 1) + up * (2 * y / YM - 1);
		return Ray(eye, p - eye);
	}
};

class Light {
public:
	vec3 p;
	vec3 Lout;
	bool type;

	Light(vec3 p = vec3(0, 0, 0), vec3 Lout = vec3(0, 0, 0)) {
		this->Lout = Lout;
		this->p = p;
	}

	vec3 getLightDir() {
		vec3 lightDir;
		return lightDir;
	}

	vec3 getInRad() {
		vec3 inRad;
		return inRad;
	}

	float getDist() {
		float dist;
		return dist;
	}

};

class Torus : public Intersectable {
public:
	vec3 center;
	float r;
	float R;
	Torus(Material* material, vec3 center, float r, float R) {
		this->material = material;
		this->center = center;
		this->r = r;
		this->R = R;
	}
	Hit intersect(const Ray& ray) {
		// TODO
		Hit h;
		return h;
	}
};

class Triangle : public Intersectable {
	vec3 a;
	vec3 b;
	vec3 c;
	Hit intersect(const Ray& ray) {
		// TODO
		Hit h;
		return h;
	}
};

class Scene {
public:
	Camera camera;
	vec3 La;
	Intersectable* objects[150];
	Light* lights[3];

	void build() {
		camera.eye = vec3(0, 0, 0);			// értelmes értékeket kellene ideírni
		camera.fov = 0.5f;					// fél radián
		La = vec3(0.01f, 0.01f, 0.01f);		// ambiens fény kicsi

		// itt kellene felépíteni a tóruszokat
		// hát gyerünk akkor baszd meg
		// ...
		// valami olyasmi, hogy először jó a tengelyek irányában
		// aztán úgy transzformáljuk őket mátrixokkal, hogy egymásba fonódjanak és ne a tengelyek irényában legyenek
		// ...
		// ok először legyen egy tórusz kevesebb, mint 50 háromszögből
		// ami legyen 6 (nagyon szögletes) * 3 (három csúcsot jelölök ki a felszínen) * 2 (mert a csúcsok négyszöget alkotnak) = 36
		// kibaszottul 36 háromszögből fog állni egy kibaszottul szögletes tórusz
		// ...
		// ok vegyük fel az első tóruszt
		// na de hogy a faszba
		// a nagy sugár legyen nagyon nagy, a kis sugár meg picike
		// de mégis mekkora a nagyon nagy?
		// R = 150, r = 20 mondjuk
		// de kellene neki anyag is!
		// legyen arany

		GoldMaterial gold;
		Torus t1(&gold, vec3(0,0,0), 20.0f, 150.0f);

		// fasza, van egy arany tórusz
		// de igazából nekem nem is tórusz kell, hanem háromszögek
		// gyártsunk háromszögeket
		// méghozzá 36-ot!
		//
		vec3 points[18];
		for (int v = 0; v < 6; v++) {
			for (int u = 0; u < 3; u++) {
				points[v * 3 + u] = vec3(
					(t1.R + t1.r * cosf((float)u/6 * 2.0f * (float)(M_PI))) * cosf((float)v/3 * 2.0f * (float)(M_PI)),
					(t1.R + t1.r * cosf((float)u/6 * 2.0f * (float)(M_PI))) * sinf((float)v/3 * 2.0f * (float)(M_PI)),
					t1.r * sinf((float)u/6 * 2.0f * (float)(M_PI))
				);
			}
		}
		// faszán lazán megvannak a csúcsok
		// és akkor most kellene vagy transzformálni ezeket és utána háromszögeket gyártani vagy fordítva
		// de már kifolyik az agyam ettől, úgyhogy inkább fixálom a trace-t és társait




	}

	void render() {

	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		for (int i = 0; i < INTERSECTABLES; i++) {	// for each intersectable object
			Hit hit = objects[i]->intersect(ray); //  hit.t < 0 if no intersection
			if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t)) {
				bestHit = hit;
			}
		}
		return bestHit;
	}

	vec3 trace(Ray ray, int depth) {
		vec3 outRadiance;
		if (depth > MAXDEPTH) {
			outRadiance = La;
		}
		else {
			outRadiance = vec3(0, 0, 0);
			Hit hit = firstIntersect(ray);
			if (hit.t < 0) {
				return La;
			}
			if (hit.material->rough) {
				outRadiance = hit.material->ka * La;
				for (int i = 0; i < LIGHTS; i++) {	// for each light

					// na hogy itt például ezek mi a tökömök
					Ray shadowRay(hit.position + hit.normal * 0.000001f * sign(hit.normal * V), L);
					Hit shadowHit = firstIntersect(shadowRay);
					if (shadowHit.t < 0 || shadowHit.t > fabs(hit.position - yl))
						outRadiance = outRadiance + hit.material->shade(hit.normal, V, Ll, Lel);
				}
			}
			if (hit.material->reflective) {
				vec3 reflectionDir = hit.material->reflect(ray.dir, hit.normal);
				Ray reflectedRay(hit.position + hit.normal * 0.000001f * sign(hit.normal * V), reflectionDir);
				outRadiance = outRadiance + trace(reflectedRay, depth + 1) * hit.material->Fresnel(V, hit.normal);
			}
			if (hit.material->refractive) {
				vec3 refractionDir = hit.material->refract(ray.dir, hit.normal);
				Ray refractedRay(hit.position - hit.normal * 0.000001f * sign(hit.normal * V), refractionDir);
				outRadiance = outRadiance + trace(refractedRay, depth + 1) * (vec3(1, 1, 1) - hit.material->Fresnel(V, hit.normal));
			}
		}
		return outRadiance;
	}
};

void getErrorInfo(unsigned int handle) {
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

// check if shader could be compiled
void checkShader(unsigned int shader, char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) {
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

// check if shader could be linked
void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) {
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

	out vec2 texcoord;

	void main() {
		texcoord = (vertexPosition + vec2(1, 1))/2;							// -1,1 to 0,1
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	in  vec2 texcoord;			// interpolated texture coordinates

	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = texture(textureUnit, texcoord); 
	}
)";


struct vec4 {
	float v[4];

	vec4(float x = 0, float y = 0, float z = 0, float w = 1) {
		v[0] = x; v[1] = y; v[2] = z; v[3] = w;
	}
};


// handle of the shader program
unsigned int shaderProgram;

class FullScreenTexturedQuad {
	unsigned int vao, textureId;	// vertex array object id and texture id
public:
	void Create(vec3 image[windowWidth * windowHeight]) {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo;		// vertex buffer objects
		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects

								// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		static float vertexCoords[] = { -1, -1, 1, -1, -1, 1,
			1, -1, 1, 1, -1, 1 };	// two triangles forming a quad
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
																							   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

																	  // Create objects by setting up their vertex data on the GPU
		glGenTextures(1, &textureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, textureId);    // binding

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, image); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		int location = glGetUniformLocation(shaderProgram, "textureUnit");
		if (location >= 0) {
			glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureId);	// connect the texture to the sampler
		}
		glDrawArrays(GL_TRIANGLES, 0, 6);	// draw two triangles forming a quad
	}
};

// The virtual world: single quad
FullScreenTexturedQuad fullScreenTexturedQuad;

vec3 background[windowWidth * windowHeight];	// The image, which stores the ray tracing result

Scene scene;
												// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	scene.build();

	// Ray tracing fills the image called background
	for (int x = 0; x < windowWidth; x++) {

		for (int y = 0; y < windowHeight; y++) {
			// TODO
			//
			//Ray r = GetRay(eye -> pixel p)
			//color = trace(r);
			//background[y * windowWidth + x] = color;
			background[y * windowWidth + x] = vec3((float)x / windowWidth, (float)y / windowHeight, 0);
		}
	}

	fullScreenTexturedQuad.Create(background);

	// Create vertex shader from string
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) {
		printf("Error in vertex shader creation\n");
		exit(1);
	}
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	// Create fragment shader from string
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) {
		printf("Error in fragment shader creation\n");
		exit(1);
	}
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "Fragment shader error");

	// Attach shaders to a single program
	shaderProgram = glCreateProgram();
	if (!shaderProgram) {
		printf("Error in shader program creation\n");
		exit(1);
	}
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Connect the fragmentColor to the frame buffer memory
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");	// fragmentColor goes to the frame buffer memory

																// program packaging
	glLinkProgram(shaderProgram);
	checkLinking(shaderProgram);
	// make this program run
	glUseProgram(shaderProgram);
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

// Window has become invalid: Redraw
void onDisplay() {
	fullScreenTexturedQuad.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow(argv[0]);

#if !defined(__APPLE__)
	glewExperimental = true;	// magic
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);                // Register event handlers
	glutMouseFunc(onMouse);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMotionFunc(onMouseMotion);

	glutMainLoop();
	onExit();
	return 1;
}