//=============================================================================================
// Szamitogepes grafika hazi feladat keret. Ervenyes 2017-tol.
// A //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sorokon beluli reszben celszeru garazdalkodni, mert a tobbit ugyis toroljuk.
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - new operatort hivni a lefoglalt adat korrekt felszabaditasa nelkul
// - felesleges programsorokat a beadott programban hagyni
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL/GLUT fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : 
// Neptun : 
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================


#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		// must be downloaded 
#include <GL/freeglut.h>	// must be downloaded unless you have an Apple
#endif

#define IMAGE 0
#define CPU_PROCEDURAL 1
#define GPU_PROCEDURAL 2

#define TEXTURING CPU_PROCEDURAL

const unsigned int windowWidth = 600, windowHeight = 600;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// You are supposed to modify the code from here...

#define CURVEDETAIL 10

// OpenGL major and minor versions
int majorVersion = 3, minorVersion = 3;

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
const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = vertexUV;														// copy texture coordinates
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
#if TEXTURING==CPU_PROCEDURAL
const char * fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	uniform vec2 texCursor;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

    vec2 LensDistortion(vec2 p) {
		const float maxRadius2 = 0.1f; 
		float radius2 = dot(texCoord - texCursor, texCoord - texCursor);
		float scale = (radius2 < maxRadius2) ? radius2 / maxRadius2 : 1;
		return (texCoord - texCursor) * scale + texCursor;
	}

	void main() {
		fragmentColor = texture(textureUnit, texCoord);
	}
)";
#endif

// row-major matrix 4x4
struct mat4 {
	float m[4][4];
public:
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
	operator float*() { return &m[0][0]; }
};


// 3D point in homogeneous coordinates
struct vec4 {
	float v[4];

	vec4(float x = 0, float y = 0, float z = 0, float w = 1) {
		v[0] = x; v[1] = y; v[2] = z; v[3] = w;
	}

	vec4 operator*(const mat4& mat) {
		vec4 result;
		for (int j = 0; j < 4; j++) {
			result.v[j] = 0;
			for (int i = 0; i < 4; i++) {
				result.v[j] += v[i] * mat.m[i][j];
			}
		}
		return result;
	}
};

// 3D point in Cartesian coordinates or RGB color
struct vec3 {
	float v[3];

	vec3(float x = 0, float y = 0, float z = 0) {
		v[0] = x; v[1] = y; v[2] = z;
	}
	vec3(const vec4& hom) {
		v[0] = hom.v[0] / hom.v[3]; v[1] = hom.v[1] / hom.v[3]; v[2] = hom.v[2] / hom.v[3];
	}
	vec3 operator*(float s) {
		vec3 res(v[0] * s, v[1] * s, v[2] * s);
		return res;
	}
	vec3 operator+(vec3 right) {
		vec3 res(v[0] + right.v[0], v[1] + right.v[1], v[2] + right.v[2]);
		return res;
	}
	vec3 operator-(vec3 right) {
		vec3 res(v[0] - right.v[0], v[1] - right.v[1], v[2] - right.v[2]);
		return res;
	}
	vec3 operator*(vec3 right) {
		vec3 res(v[0] * right.v[0], v[1] * right.v[1], v[2] * right.v[2]);
		return res;
	}
	vec3 operator/(float s) {
		vec3 res(v[0], v[1], v[2]);
		if (s != 0.0f) {
			res.v[0] = v[0] / s;
			res.v[1] = v[1] / s;
			res.v[2] = v[2] / s;
		}
		return res;
	}
	void operator=(vec3 right) {
		v[0] = right.v[0];
		v[1] = right.v[1];
		v[2] = right.v[2];
	}
	float Length() {
		return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	}
};

float dot(vec3 v1, vec3 v2) {
	return (v1.v[0] * v2.v[0] + v1.v[1] * v2.v[1] + v1.v[2] * v2.v[2]);
}

// 2D point in Cartesian coordinates
struct vec2 {
	float v[2];

	vec2(float x = 0, float y = 0) {
		v[0] = x; v[1] = y;
	}
	vec2(const vec4& hom) {
		v[0] = hom.v[0] / hom.v[3]; v[1] = hom.v[1] / hom.v[3];
	}
	vec2 operator-(vec2& right) {
		vec2 res(v[0] - right.v[0], v[1] - right.v[1]);
		return res;
	}
	float Length() {
		return sqrtf(v[0] * v[0] + v[1] * v[1]);
	}
};


// 2D camera
struct Camera {
	float wCx, wCy;	// center in world coordinates
	float wWx, wWy;	// width and height in world coordinates
public:
	Camera() {
		Animate(0);
	}

	mat4 V() { // view matrix: translates the center to the origin
		return mat4(1,		0,		0,		0,
					0,		1,		0,		0,
					0,		0,		1,		0,
					-wCx,	-wCy,	0,		1);
	}

	mat4 P() { // projection matrix: scales it to be a square of edge length 1
		return mat4(2 / wWx,	0,			0,			0,
					0,			2 / wWy,	0,			0,
					0,			0,			1,			0,
					0,			0,			0,			1);
	}

	mat4 Vinv() { // inverse view matrix
		return mat4(1,		0,		0,		0,
					0,		1,		0,		0,
					0,		0,		1,		0,
					wCx,	wCy,	0,		1);
	}

	mat4 Pinv() { // inverse projection matrix
		return mat4(wWx / 2,	0,			0,			0,
					0,			wWy / 2,	0,			0,
					0,			0,			1,			0,
					0,			0,			0,			1);
	}

	void Animate(float t) {
		wCx = 0.5f;
		wCy = 0.5f;
		wWx = 1.0f;
		wWy = 1.0f;
	}
};

// 2D camera
Camera camera;

// handle of the shader program
unsigned int shaderProgram;

class LineStrip {
	GLuint vao, vbo, lineTextureId;						// vertex array object, vertex buffer object, textureId
	float  vertexData[(3*CURVEDETAIL+1)*2];				// data of coordinates
	int    nVertices;									// number of vertices
public:
	LineStrip() {
		nVertices = 0;
	}
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate 1 vertex buffer object
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0
		
		// Map attribute array 0 to the vertex data of the vbo
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0)); // attribute array, components/attribute, component type, normalize?, stride, offset

		// TEXTURE
		int width = 1;
		int height = 1;
		vec3 * image = new vec3[width * height];
		image[0] = vec3(1.0f, 1.0f, 1.0f);

		glGenTextures(1, &lineTextureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, lineTextureId);    // binding

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_FLOAT, image); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void AddPoint(float cX, float cY) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		if (nVertices >= 31) {
			return;
		}
		// fill  data
		vertexData[2 * nVertices] = cY;
		vertexData[2 * nVertices + 1] = cX;
		nVertices++;
		// copy data to the GPU
		glBufferData(GL_ARRAY_BUFFER, nVertices * 2 * sizeof(float), vertexData, GL_DYNAMIC_DRAW);
	}

	void AddPoint(vec3 v) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		if (nVertices >= 31) {
			return;
		}
		// fill  data
		vertexData[2 * nVertices] = v.v[0];
		vertexData[2 * nVertices + 1] = v.v[1];
		nVertices++;
		// copy data to the GPU
		glBufferData(GL_ARRAY_BUFFER, nVertices * 2 * sizeof(float), vertexData, GL_DYNAMIC_DRAW);
	}

	void Draw() {
		if (nVertices > 0) {

			glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source

			mat4 VPTransform = camera.V() * camera.P();

			int location = glGetUniformLocation(shaderProgram, "MVP");
			if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, VPTransform);
			else printf("uniform MVP cannot be set\n");

#if TEXTURING==CPU_PROCEDURAL
			location = glGetUniformLocation(shaderProgram, "textureUnit");
			if (location >= 0) {
				glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lineTextureId);	// connect the texture to the sampler
			}
#endif

			glDrawArrays(GL_LINE_STRIP, 0, nVertices);
		}
	}

	void setVertices(int n) {
		nVertices = n;
	}

};

class Torus {
private:
	LineStrip lineStrip;
	vec3 tp[3];
	vec3 cp[3];
	int points;
	float r;
	float R;

public:
	Torus() {
		r = 1.0f;
		R = 2.0f;
		points = 0;
	}

	void Create() {
		lineStrip.Create();
	}

	void addPoint(float u, float v) {

		// start a new triangle after 3 clicks
		switch (points) {
		case 0:
			tp[points].v[0] = u;
			tp[points].v[1] = v;
			points++;
			break;

		case 1:
			tp[points].v[0] = u;
			tp[points].v[1] = v;
			points++;
			createCurve(0, 1);
			break;

		case 2:
			tp[points].v[0] = u;
			tp[points].v[1] = v;
			points++;
			createCurve(1, 2);
			createCurve(2, 0);
			break;

		case 3:
			points = 0;
			lineStrip.setVertices(0);
			tp[points].v[0] = u;
			tp[points].v[1] = v;
			points++;
			break;

		default:
			break;
		}
	}

	void createCurve(int i1, int i2) {
		vec3 p;		
		// initial state of control point
		p.v[0] = (tp[i1].v[0] + tp[i2].v[0]) / 2;
		p.v[1] = (tp[i1].v[1] + tp[i2].v[1]) / 2;

		float E = getLength(tp[i1], p, tp[i2]);
		float minE = E;
		vec3 minp = p;

		float kT = 10;
		
		// simulated annealing
		while (kT > 0.01f) {
			vec3 pvesszo = Perturb(p);

			float Evesszo = getLength(tp[i1], pvesszo, tp[i2]);
			if (random() < exp(-(Evesszo - E) / kT)) {
				E = Evesszo;
				p = pvesszo;
				if (Evesszo < minE) {
					minE = Evesszo;
					minp = pvesszo;
				}
			}
			kT *= (1.0f - 0.0001f);
		}
		
		// add points to linestrip
		if (i1 == 0) {
			for (int i = 0; i < CURVEDETAIL + 1; i++) {
				vec3 p = tp[i1] * pow((1.0f - ((float)i / (float)(CURVEDETAIL))), 2) + minp * 2.0f * ((float)i / CURVEDETAIL) * ((float)1 - (float)i / CURVEDETAIL) + tp[i2] * pow((float)i / CURVEDETAIL, 2);
				lineStrip.AddPoint(p);
				cp[i1] = minp;
			}
		}

		else {
			for (int i = 1; i < CURVEDETAIL + 1; i++) {
				vec3 p = tp[i1] * pow((1.0f - (float)i / CURVEDETAIL), 2) + minp * 2.0f * ((float)i / CURVEDETAIL) * ((float)1 - (float)i / CURVEDETAIL) + tp[i2] * pow((float)i / CURVEDETAIL, 2);
				lineStrip.AddPoint(p);
				cp[i1] = minp;
			}
		}
		if(i1 == 2) {
			sumOfAngles();
		}
	}

	void sumOfAngles() {
		float angleSumRad = 0.0f;
		//convert to 3D
		for (int i = 0; i < 3; i++) {
			float cX, cY, cZ;
			cX = x(cp[i]);
			cY = y(cp[i]);
			cZ = z(cp[i]);
			cp[i].v[0] = cX;
			cp[i].v[1] = cY;
			cp[i].v[2] = cY;

			cX = x(tp[i]);
			cY = y(tp[i]);
			cZ = z(tp[i]);
			tp[i].v[0] = cX;
			tp[i].v[1] = cY;
			tp[i].v[2] = cY;
		}

		//calculate angle
		for (int i = 0; i < 3; i++) {
			vec3 v1 = cp[(i-1+3)%3] - tp[i];
			vec3 v2 = cp[i] - tp[i];

			float v1mag = v1.Length();
			float v2mag = v2.Length();

			vec3 v1norm = v1 / v1mag;
			vec3 v2norm = v2 / v2mag;
			float res = dot(v1norm, v2norm);
			angleSumRad += acosf(res);
		}

		printf("%f\n", angleSumRad);
	}

	// normalized random value
	float random() {
		return (((float)(rand())) / (float)(RAND_MAX));
	}

	// move point randomly
	vec3 Perturb(vec3 oldc) {
		vec3 c;
		switch (rand() % 4) {
		case 0:
			c.v[0] = oldc.v[0] + 0.001f;
			c.v[1] = oldc.v[1];
			break;
		case 1:
			c.v[0] = oldc.v[0] - 0.001f;
			c.v[1] = oldc.v[1];
			break;
		case 2:
			c.v[0] = oldc.v[0];
			c.v[1] = oldc.v[1] + 0.001f;
			break;
		default:
			c.v[0] = oldc.v[0];
			c.v[1] = oldc.v[1] - 0.001f;
			break;
		}

		return c;
	}

	float getLength(vec3 p0, vec3 c, vec3 p1) {

		float length = 0.0f;
		
		vec3 curvePoints2D[CURVEDETAIL + 1];

		for (int i = 0; i < CURVEDETAIL + 1; i++) {
			curvePoints2D[i] = p0 * pow((1.0f - (float)i / CURVEDETAIL), 2) + c * 2.0f * ((float)i / CURVEDETAIL) * ((float)1 - (float)i / CURVEDETAIL) + p1 * pow((float)i / CURVEDETAIL, 2);
		}

		vec3 curvePoints3D[CURVEDETAIL + 1];

		for (int i = 0; i < CURVEDETAIL + 1; i++) {
			curvePoints3D[i].v[0] = x(curvePoints2D[i]);
			curvePoints3D[i].v[1] = y(curvePoints2D[i]);
			curvePoints3D[i].v[2] = z(curvePoints2D[i]);
		}

		for (int i = 0; i < CURVEDETAIL; i++) {
			vec3 temp = curvePoints3D[i + 1] - curvePoints3D[i];
			length += temp.Length();
		}
		return length;
	}

	// convert 2D to 3D
	// x = (R + r * cos(u))*cos(v)
	float x(vec3 point2D) {
		float cX;
		cX = ((float)R + (float)r * cosf(point2D.v[0] * 2.0f * (float)(M_PI))) * cosf(point2D.v[1] * 2.0f * (float)(M_PI));
		return cX;
	}

	// y = (R + r * cos(u))*sin(v)
	float y(vec3 point2D) {
		float cY;
		cY = ((float)R + (float)r * cosf(point2D.v[0] * 2.0f * (float)(M_PI))) * sinf(point2D.v[1] * 2.0f * (float)(M_PI));
		return cY;
	}

	// z = r * sin(u)
	float z(vec3 point2D) {
		float cZ;
		cZ = r * sinf(point2D.v[0] * 2.0f * (float)(M_PI));
		return cZ;
	}

	void Draw() {
		lineStrip.Draw();
	}

};


class TexturedQuad {
	unsigned int vao, vbo[2], quadTextureId;	// vertex array object id and texture ids
	vec2 vertices[4], uvs[4];
public:
	TexturedQuad() {
		vertices[0] = vec2(0, 1); uvs[0] = vec2(0, 0);
		vertices[1] = vec2(1, 1);  uvs[1] = vec2(1, 0);
		vertices[2] = vec2(1, 0);   uvs[2] = vec2(1, 1);
		vertices[3] = vec2(0, 0);  uvs[3] = vec2(0, 1);
	}
	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		glGenBuffers(2, vbo);	// Generate 2 vertex buffer objects

								// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);	   // copy to that part of the memory which is not modified 
																					   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
																			   // Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

#if TEXTURING==CPU_PROCEDURAL
		int width = 600, height = 600;
		float r = 1.0f;
		float R = 2.0f;
		float mingauss = (1.0f / r) * (-1.0f) / (R + r * (-1.0f));
		float maxgauss = (1.0f / r) * (1.0f) / (R + r * (1.0f));
		float gauss;
		float r_luminance = 0.0f;
		float g_luminance = 0.0f;
		float b_luminance = 0.0f;
		vec3 * image = new vec3[width * height];
		for (int u = 0; u < width; u++) {
			gauss = (1.0f / r) * cosf((float)u / width * 2 * 3.141592f) / (R + r * cosf((float)u / width * 2 * 3.141592f));
			if (gauss > 0) {
				r_luminance = gauss / maxgauss;
				g_luminance = 0.0f;
				b_luminance = 0.0f;
			}
			else {
				r_luminance = 0.0f;
				g_luminance = gauss / mingauss;
				b_luminance = 0.0f;
			}
			image[0 * width + u] = vec3(r_luminance, g_luminance, b_luminance);
		}

		for (int v = 1; v < height; v++) {
			for (int u = 0; u < width; u++) {
				image[v * width + u] = image[0 * width + u];
			}
		}

		// Create objects by setting up their vertex data on the GPU
		glGenTextures(1, &quadTextureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, quadTextureId);    // binding

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_FLOAT, image); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source

		mat4 MVPTransform = camera.V() * camera.P();

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

#if TEXTURING==CPU_PROCEDURAL
		location = glGetUniformLocation(shaderProgram, "textureUnit");
		if (location >= 0) {
			glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, quadTextureId);	// connect the texture to the sampler
		}
#endif
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};

// The virtual world: collection of objects

TexturedQuad quad;
Torus torus;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	// seed rand() method
	srand(1654);

	// Create objects by setting up their vertex data on the GPU
	quad.Create();
	torus.Create();

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
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen

	quad.Draw();
	torus.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

bool mouseLeftPressed = false;
bool mouseRightPressed = false;

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
	return;
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			mouseLeftPressed = true;
			float cX = (float)pX / windowWidth;
			// flip y axis
			float cY = 1.0f - (float)pY / windowHeight;
			torus.addPoint(cX, cY);
		}
		else					mouseLeftPressed = false;
	}
	if (button == GLUT_RIGHT_BUTTON) {
		if (state == GLUT_DOWN) mouseRightPressed = true;
		else					mouseRightPressed = false;
	}
}


// Idle event indicating that some time elapsed: do animation here
void onIdle() {
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Do not touch the code below this line

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_3_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
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