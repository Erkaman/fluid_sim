
#include <cstdlib>

#include <cstring>
#include <string>

#include <vector>
#include <string>

#include <math.h>

#include <chrono>
#include <thread>


#include <glad/glad.h>

#include <GLFW/glfw3.h>

GLFWwindow* window;

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)


inline void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("OpenGL error %08x, at %s:%i - for %s.\n", err, fname, line, stmt);
	}
}

#ifdef NDEBUG
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
    } while (0)
#else
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
	CheckOpenGLError(#stmt, __FILE__, __LINE__);	\
    } while (0)
#endif

inline char* GetShaderLogInfo(GLuint shader) {
	GLint len;
	GLsizei actualLen;
	GL_C(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
	char* infoLog = new char[len];
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));
	return infoLog;
}

inline GLuint CreateShaderFromString(const std::string& shaderSource, const GLenum shaderType) {
	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));
	const char *c_str = shaderSource.c_str();
	GL_C(glShaderSource(shader, 1, &c_str, NULL));
	GL_C(glCompileShader(shader));

	GLint compileStatus;
	GL_C(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
	if (compileStatus != GL_TRUE) {
		LOGI("Could not compile shader\n\n%s \n\n%s\n", shaderSource.c_str(),
			GetShaderLogInfo(shader));
		exit(1);
	}

	return shader;
}


inline GLuint LoadNormalShader(const std::string& vsSource, const std::string& fsShader) {

	std::string prefix = "";
	prefix = "#version 330\n";

	GLuint vs = CreateShaderFromString(prefix + vsSource, GL_VERTEX_SHADER);
	GLuint fs = CreateShaderFromString(prefix + fsShader, GL_FRAGMENT_SHADER);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint Result;
	glGetProgramiv(shader, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		LOGI("Could not link shader \n\n%s\n", GetShaderLogInfo(shader));
		exit(1);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
}

class vec2 {
public:
	float x, y;

	vec2(float x, float y) { this->x = x; this->y = y; }

	vec2() { this->x = this->y = 0; }
};

//
//
// Begin vec3.
//
//

class vec3 {
public:
	float x, y, z;

	vec3(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	vec3() { this->x = this->y = this->z = 0; }

	vec3& operator+=(const vec3& b) { (*this) = (*this) + b; return (*this); }

	friend vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	friend vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	friend vec3 operator*(const float s, const vec3& a) { return vec3(s * a.x, s * a.y, s * a.z); }
	friend vec3 operator*(const vec3& a, const float s) { return s * a; }

	static float length(const vec3& a) { return sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }

	static vec3 mix(const vec3& x, const vec3& y, const float a) { return x * (1.0f - a) + y * a; }


	// cross product.
	static vec3 cross(const vec3& a, const vec3& b) { return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	//
	static vec3 rotate(const vec3& v, const float theta, const vec3& axis) {
		vec3 k = vec3::normalize(axis); // normalize for good measure.
		return v * cos(theta) + vec3::cross(k, v)* sin(theta) + (k * vec3::dot(k, v)) * (1.0f - cos(theta));
	}

};

class vec4 {
public:
	float x, y, z, w;

	vec4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }

	vec4() { this->x = this->y = this->z = 0; this->w = 0; }

	vec4& operator+=(const vec4& b) { (*this) = (*this) + b; return (*this); }

	friend vec4 operator-(const vec4& a, const vec4& b) { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	friend vec4 operator+(const vec4& a, const vec4& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	friend vec4 operator*(const float s, const vec4& a) { return vec4(s * a.x, s * a.y, s * a.z, s * a.w); }
	friend vec4 operator*(const vec4& a, const float s) { return s * a; }
	/*
	static float length(const vec3& a) { return sqrt(vec3::dot(a, a)); }

	// dot product.
	static float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	static float distance(const vec3& a, const vec3& b) { return length(a - b); }
	static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }
	*/
	//
	// Rotate the vector 'v' around the 'axis' for 'theta' degrees.
	// This is basically Rodrigues' rotation formula.
	//
};


const int WINDOW_WIDTH = 256*3;
const int WINDOW_HEIGHT = 256*3;

GLuint vao;

//float timeStep = 1.0 / 60.0f;

int fbWidth, fbHeight;

int frameW, frameH;

struct FullscreenVertex {
	float x, y; // position
};

//vec2 delta;


GLuint fullscreenVertexVbo;

GLuint advectShader;
GLuint asuTexLocation;
GLuint assTexLocation;

GLuint jacobiShader;
GLuint jsxTexLocation;
GLuint jsbTexLocation;
GLuint jsAlphaLocation;
GLuint jsBetaLocation;

GLuint divergenceShader;
GLuint dswTexLocation;


GLuint visShader;
GLuint vsTexLocation;

GLuint gradientSubtractionShader;
GLuint gsspTexLocation;
GLuint gsswTexLocation;

GLuint forceShader;
GLuint fswTexLocation;
GLuint fsForceLocation;
GLuint fsPosLocation;

GLuint boundaryShader;
GLuint bsPosOffsetLocation;
GLuint bsPosSizeLocation;
GLuint bswTexSizeLocation;
GLuint bsSampleOffsetLocation;
GLuint bsScaleLocation;

GLuint fbo0;
GLuint fbo1;

// velocity.
GLuint uBegTex;
GLuint uEndTex;

// color.
GLuint cBegTex;
GLuint cEndTex;

GLuint wTex;
GLuint wTempTex;
GLuint wDivergenceTex;
GLuint pTempTex[2];
GLuint pTex;
GLuint debugTex;
GLuint uEndTempTex;

GLuint debugDataTex;
GLuint debugDataTex2;

void error_callback(int error, const char* description)
{
	puts(description);
}

#ifdef WIN32
void InitGlfw() {
	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Deferred Shading Demo", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	glfwSetWindowPos(window, 100, 100);


	// load GLAD.
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Bind and create VAO, otherwise, we can't do anything in OpenGL.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
}
#endif

void RenderFullscreen() {
	GL_C(glEnableVertexAttribArray((GLuint)0));
	GL_C(glBindBuffer(GL_ARRAY_BUFFER, fullscreenVertexVbo));
	GL_C(glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, sizeof(FullscreenVertex), (void*)0));

	GL_C(glDrawArrays(GL_TRIANGLES, 0, 6));
}

void clearTexture(GLuint tex) {
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
	GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0));
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
	{
		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT));
	}
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void computeDivergence(GLuint src, GLuint dst) {
	// compute divergence of w.
	{
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		{
			GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GL_C(glClear(GL_COLOR_BUFFER_BIT));

			GL_C(glUseProgram(divergenceShader));

			GL_C(glUniform1i(dswTexLocation, 0));
			GL_C(glActiveTexture(GL_TEXTURE0 + 0));
			GL_C(glBindTexture(GL_TEXTURE_2D, src));

			RenderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
}

void advect(GLuint src, GLuint u, GLuint dst) {
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
	GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0));
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
	{
		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT));

		GL_C(glUseProgram(advectShader));

		GL_C(glUniform1i(asuTexLocation, 0));
		GL_C(glActiveTexture(GL_TEXTURE0 + 0));
		GL_C(glBindTexture(GL_TEXTURE_2D, u));

		GL_C(glUniform1i(assTexLocation, 1));
		GL_C(glActiveTexture(GL_TEXTURE0 + 1));
		GL_C(glBindTexture(GL_TEXTURE_2D, src));

		RenderFullscreen();
	}
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

GLuint  jacobi(const int nIter, GLuint bTex, GLuint* tempTex) {
	int iter;
	for (iter = 0; iter < nIter; ++iter) {
		int curJ = (iter + 0) % 2;
		int nextJ = (iter + 1) % 2;

		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempTex[nextJ], 0));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		{
			GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GL_C(glClear(GL_COLOR_BUFFER_BIT));

			GL_C(glUseProgram(jacobiShader));

			GL_C(glUniform1i(jsxTexLocation, 0));
			GL_C(glActiveTexture(GL_TEXTURE0 + 0));
			GL_C(glBindTexture(GL_TEXTURE_2D, tempTex[curJ]));

			GL_C(glUniform1i(jsbTexLocation, 1));
			GL_C(glActiveTexture(GL_TEXTURE0 + 1));
			GL_C(glBindTexture(GL_TEXTURE_2D, bTex));

			//GL_C(glUniform2f(jsAlphaLocation, -delta.x*delta.x, -delta.y*delta.y));
			GL_C(glUniform2f(jsAlphaLocation, -1.0f, -1.0f));

			GL_C(glUniform2f(jsBetaLocation, 1.0 / 4.0, 1.0 / 4.0));


			RenderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	
	// now we have the pressure:
	return tempTex[(iter + 0) % 2];
}

#define GL_DEBUG_SOURCE_APPLICATION       0x824A
void renderFrame() {

	// StartTimingEvent();

	static bool firstFrame = true;
	// setup matrices.

	//profiler->Begin(GpuTimeStamp::RENDER_EVERYTHING);

	// setup GL state.
	GL_C(glDisable(GL_DEPTH_TEST));
	GL_C(glDepthMask(false));
	GL_C(glDisable(GL_BLEND));
	GL_C(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
	GL_C(glEnable(GL_CULL_FACE));
	GL_C(glFrontFace(GL_CCW));
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_C(glUseProgram(0));
	GL_C(glBindTexture(GL_TEXTURE_2D, 0));
	GL_C(glDepthFunc(GL_LESS));

	GL_C(glViewport(0, 0, frameW, frameH));
	
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Clear pTemp Textures");
	clearTexture(pTempTex[0]);
	clearTexture(pTempTex[1]);
	glad_glPopDebugGroup();
	
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "c Advection");
	advect(cBegTex, uBegTex, cEndTex);
	glad_glPopDebugGroup();
	
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "u Advection");
	advect(uBegTex, uBegTex, wTex);
	glad_glPopDebugGroup();

	static int counter = 0;
	counter++;

	// add force.
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "c Add Force");
	{
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
			
			wTempTex,
			//uEndTex,
			
			0));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		{
			GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GL_C(glClear(GL_COLOR_BUFFER_BIT));

			GL_C(glUseProgram(forceShader));
			
			GL_C(glUniform1i(fswTexLocation, 0));
			GL_C(glActiveTexture(GL_TEXTURE0 + 0));
			GL_C(glBindTexture(GL_TEXTURE_2D, wTex));
			
			if (counter == 10) {
				GL_C(glUniform2f(fsPosLocation, 0.5f, 0.5f));		   
				GL_C(glUniform2f(fsForceLocation, +0.0f, +4.5f));
			}
			else {

				GL_C(glUniform2f(fsPosLocation, 0.5f, 0.5f));
				GL_C(glUniform2f(fsForceLocation, 0.0f, 4.5f));
			}

			RenderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	glad_glPopDebugGroup();

	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Pressure Gradient Subtract");
	// subtraction of pressure gradient.
	{
		glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Compute divergence of w");
		computeDivergence(wTempTex, wDivergenceTex);
		glad_glPopDebugGroup();

		glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Compute pressure");
		// compute pressure.
		{

			glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Clear pTemp Textures");
			clearTexture(pTempTex[0]);
			clearTexture(pTempTex[1]);
			glad_glPopDebugGroup();

			glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Jacobi");

			pTex = jacobi(40,
				wDivergenceTex, // b
				pTempTex
			);
			
			glad_glPopDebugGroup();

		}
		glad_glPopDebugGroup();

	//	I think maybe the texture sampling coordinates are wrong. try not samplignthe texel center.

		glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "pressure gradient subtraction");
		{
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, uEndTex, 0));
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			{
				GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
				GL_C(glClear(GL_COLOR_BUFFER_BIT));

				GL_C(glUseProgram(gradientSubtractionShader));
				
				GL_C(glUniform1i(gsswTexLocation, 0));
				GL_C(glActiveTexture(GL_TEXTURE0 + 0));
				GL_C(glBindTexture(GL_TEXTURE_2D, wTempTex));

				GL_C(glUniform1i(gsspTexLocation, 1));
				GL_C(glActiveTexture(GL_TEXTURE0 + 1));
				GL_C(glBindTexture(GL_TEXTURE_2D, pTex));

				RenderFullscreen();
			}
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		glad_glPopDebugGroup();

	}
	glad_glPopDebugGroup();
	
	/*
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Boudary Block");
	{
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			uEndTex, 0	));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		{
			GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GL_C(glClear(GL_COLOR_BUFFER_BIT));

			GL_C(glUseProgram(boundaryShader));
			
			GL_C(glUniform1i(bswTexSizeLocation, 1));
			GL_C(glActiveTexture(GL_TEXTURE0 + 1));
			GL_C(glBindTexture(GL_TEXTURE_2D, uEndTempTex));

			vec2 delta = vec2(1.0f / float(frameW), 1.0f / float(frameH));

			GL_C(glUniform2f(bsPosOffsetLocation, delta.x, delta.y));
			GL_C(glUniform2f(bsPosSizeLocation, 1.0 - 2.0 * delta.x, 1.0 - 2.0 * delta.y));
			GL_C(glUniform2f(bsSampleOffsetLocation, 0.0f, 0.0f));
			GL_C(glUniform1f(bsScaleLocation, 1.0f));

			RenderFullscreen();

		
			GL_C(glUniform2f(bsPosOffsetLocation, 0.0f, 0.0f));
			GL_C(glUniform2f(bsPosSizeLocation, 1.0f, delta.y));
			GL_C(glUniform2f(bsSampleOffsetLocation, 0.0f, +delta.y));
			GL_C(glUniform1f(bsScaleLocation, -1.0f));
			RenderFullscreen();
			
			GL_C(glUniform2f(bsPosOffsetLocation, 0.0f, 1.0f - delta.y));
			GL_C(glUniform2f(bsPosSizeLocation, 1.0f, delta.y));
			GL_C(glUniform2f(bsSampleOffsetLocation, 0.0f, -delta.y));
			GL_C(glUniform1f(bsScaleLocation, -1.0f));
			RenderFullscreen();
			
			GL_C(glUniform2f(bsPosOffsetLocation, 0.0f, 0.0f));
			GL_C(glUniform2f(bsPosSizeLocation, delta.x, 1.0f));
			GL_C(glUniform2f(bsSampleOffsetLocation, +delta.x, 0.0f));
			GL_C(glUniform1f(bsScaleLocation, -1.0f));
			RenderFullscreen();
			
			GL_C(glUniform2f(bsPosOffsetLocation, 1.0f - delta.x, 0.0));
			GL_C(glUniform2f(bsPosSizeLocation, delta.x, 1.0f));
			GL_C(glUniform2f(bsSampleOffsetLocation, -delta.x, 0.0f));	
			GL_C(glUniform1f(bsScaleLocation, -1.0f));
			RenderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	glad_glPopDebugGroup();
	*/
	
	
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Rendering");
	{
		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT));

		GL_C(glUseProgram(visShader));

		GL_C(glUniform1i(vsTexLocation, 0));
		GL_C(glActiveTexture(GL_TEXTURE0 + 0));
		GL_C(glBindTexture(GL_TEXTURE_2D, cEndTex));

		RenderFullscreen();
	}
	glad_glPopDebugGroup();
	
	//if (counter <= 30000) 
	{

		// swap.
		GLuint temp = uBegTex;
		uBegTex = uEndTex;
		uEndTex = temp;

		temp = cBegTex;
		cBegTex = cEndTex;
		cEndTex = temp;
	}
}

#ifdef WIN32
void HandleInput() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}
#endif

void checkFbo() {
	// make sure nothing went wrong:
	GLenum status;
	GL_C(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("Framebuffer not complete. Status: 0x%08x\n", status);
	}
}

GLuint createFloatTexture(float* data) {
	GLuint tex;

	GL_C(glGenTextures(1, &tex));
	GL_C(glBindTexture(GL_TEXTURE_2D, tex));
	GL_C(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, frameW, frameH, 0, GL_RGBA, GL_FLOAT, data));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GL_C(glBindTexture(GL_TEXTURE_2D, 0));

	return tex;
}

void setupGraphics(int w, int h) {

#ifdef WIN32
	InitGlfw();
#else
	fbWidth = w;
	fbHeight = h;
#endif

	frameW = fbWidth;
	frameH = fbHeight;

	//delta = vec2(1.0f / frameW, 1.0f / frameH);

	// frameFbo
	{
		{
			

			float* cData = new float[frameW * frameH * 4];
			for (int y = 0; y < frameH; ++y) {
				for (int x = 0; x < frameW; ++x) {
					float fx = (float)x / (float)frameW;
					float fy = (float)y / (float)frameH;

					float r = 1.0f;
					float g = 0.0f;
					float b = 0.0f;


					if (fx > 0.5 && fy > 0.5) {
						float r = 0.0f;
						float g = 1.0f;
						float b = 0.0f;
					}

					r = fx;
					g = fy;;
					b = fmaxf(fx, fy);

					cData[4 * (frameW * y + x) + 0] = r;			
					cData[4 * (frameW * y + x) + 1] = g;			
					cData[4 * (frameW * y + x) + 2] = b;
					cData[4 * (frameW * y + x) + 3] = 1.0f;

				}
			}
			cBegTex = createFloatTexture(cData);

			float* zeroData = new float[frameW * frameH * 4];
			for (int y = 0; y < frameH; ++y) {
				for (int x = 0; x < frameW; ++x) {

					zeroData[4 * (frameW * y + x) + 0] = 0.0f;
					zeroData[4 * (frameW * y + x) + 1] = 0.0f;			
					zeroData[4 * (frameW * y + x) + 2] = 0.0f;
					zeroData[4 * (frameW * y + x) + 3] = 0.0f;
				}
			}
			uBegTex = createFloatTexture(zeroData);
			uEndTex = createFloatTexture(zeroData);
			cEndTex = createFloatTexture(zeroData);
			wTex = createFloatTexture(zeroData);
			wTempTex = createFloatTexture(zeroData);
			wDivergenceTex = createFloatTexture(zeroData);
			debugTex = createFloatTexture(zeroData);
			uEndTempTex = createFloatTexture(zeroData);

			pTempTex[0] = createFloatTexture(zeroData);
			pTempTex[1] = createFloatTexture(zeroData);

			float r = 1.0f / (1.0f);
			//float r = 1.0f;

			zeroData[4 * (frameW * 10 + 10) + 0] = (-4.0 * 5.0f) * r;

			zeroData[4 * (frameW * 11 + 10) + 0] = 5.0f * r;
			zeroData[4 * (frameW * 9  + 10) + 0] = 5.0f * r;
			zeroData[4 * (frameW * 10 + 11) + 0] = 5.0f * r;
			zeroData[4 * (frameW * 10 + 9 ) + 0] = 5.0f * r;

			debugDataTex = createFloatTexture(zeroData);
			debugDataTex2 = createFloatTexture(zeroData);
		}

		{
			GL_C(glGenFramebuffers(1, &fbo0));
			GL_C(glGenFramebuffers(1, &fbo1));
		}
	}
	
	std::string fullscreenVs(R"(
       layout(location = 0) in vec3 vsPos;

        out vec2 fsUv;

        void main() {
          fsUv = remapPos(vsPos.xy);
          gl_Position =  vec4(2.0 * remapPos(vsPos.xy) - vec2(1.0), 0.0, 1.0);
        }
		)");


	std::string deltaCode = std::string("const vec2 delta = vec2(") + std::to_string(1.0f / float(frameW)) + std::string(",") + std::to_string(1.0f / float(frameH)) + ");\n";

	std::string fragDefines = "";
	fragDefines += deltaCode;
	
	std::string remapPosCode = std::string("vec2 remapPos(vec2 p) { return delta + p * (1.0 - 2.0 * delta); }\n");
	//std::string remapUvCode = std::string("vec2 remapUv(vec2 p) { return 1.5*delta + p * (1.0 - 3.0 * delta); }\n");

	std::string vertDefines = "";
	vertDefines += deltaCode;
	vertDefines += remapPosCode;
//	vertDefines += remapUvCode;

	
	advectShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D uuTex;
        uniform sampler2D usTex;

        out vec4 FragColor;

		void main()
		{
          vec2 tc = fsUv - delta * texture(uuTex, fsUv).xy;
          FragColor = texture(usTex, tc);

		}
		)")
	);
	asuTexLocation = glGetUniformLocation(advectShader, "uuTex");
	assTexLocation = glGetUniformLocation(advectShader, "usTex");

	jacobiShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D uxTex;
        uniform sampler2D ubTex;

        uniform vec2 uBeta;
        uniform vec2 uAlpha;

        out vec4 FragColor;

		void main()
		{
          vec4 xR = texture(uxTex, fsUv + vec2(+1, +0) * delta);
          vec4 xL = texture(uxTex, fsUv + vec2(-1, +0) * delta);
          vec4 xT = texture(uxTex, fsUv + vec2(+0, +1) * delta);
          vec4 xB = texture(uxTex, fsUv + vec2(+0, -1) * delta);

          vec4 bC = texture(ubTex, fsUv);

          FragColor = (xR + xL + xT + xB + vec4(uAlpha.xy, 0.0, 0.0) * bC) * vec4(uBeta.xy, 0.0, 0.0);
		}
		)")
	);
	jsxTexLocation = glGetUniformLocation(jacobiShader, "uxTex");
	jsbTexLocation = glGetUniformLocation(jacobiShader, "ubTex");
	jsBetaLocation = glGetUniformLocation(jacobiShader, "uBeta");
	jsAlphaLocation = glGetUniformLocation(jacobiShader, "uAlpha");
	
	divergenceShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D uwTex;

        out vec4 FragColor;
	
		void main()
		{
          vec4 wR = texture(uwTex, fsUv + vec2(+1, +0) * delta);
          vec4 wL = texture(uwTex, fsUv + vec2(-1, +0) * delta);
          vec4 wT = texture(uwTex, fsUv + vec2(+0, +1) * delta);
          vec4 wB = texture(uwTex, fsUv + vec2(+0, -1) * delta);

          FragColor = vec4(0.5 * (wR.x - wL.x) + 0.5 * (wT.y - wB.y));
       
		}
		)")
	);
	dswTexLocation = glGetUniformLocation(divergenceShader, "uwTex");

	gradientSubtractionShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D upTex;
        uniform sampler2D uwTex;

        out vec4 FragColor;

		void main()
		{
          vec4 pR = texture(upTex, fsUv + vec2(+1, +0) * delta);
          vec4 pL = texture(upTex, fsUv + vec2(-1, +0) * delta);
          vec4 pT = texture(upTex, fsUv + vec2(+0, +1) * delta);
          vec4 pB = texture(upTex, fsUv + vec2(+0, -1) * delta);

          vec4 c =  texture(uwTex, fsUv);
          c.xy -= vec2(0.5 * (pR.x - pL.x), 0.5 * (pT.y - pB.y));
          FragColor = c;
		}
		)")
	);
	gsspTexLocation = glGetUniformLocation(gradientSubtractionShader, "upTex");
	gsswTexLocation = glGetUniformLocation(gradientSubtractionShader, "uwTex");

	forceShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D uwTex;
        uniform vec2 uForce;
        uniform vec2 uPos;

        out vec4 FragColor;

		void main()
		{
          vec2 F = vec2(0.0, 0.0);
          float dist = distance(fsUv, uPos);
          if(dist < 0.05) {
            F += ((0.05 - dist)/0.05) * uForce;
          }

          FragColor = vec4(F.xy, 0.0, 0.0) + texture(uwTex, fsUv);
		}
		)")
	);
	fswTexLocation = glGetUniformLocation(forceShader, "uwTex");
	fsForceLocation = glGetUniformLocation(forceShader, "uForce");
	fsPosLocation = glGetUniformLocation(forceShader, "uPos");
	

	boundaryShader = LoadNormalShader(
		
R"(
	uniform vec2 uPosOffset;
	uniform vec2 uPosSize;

	vec2 remapPos(vec2 p) { return uPosOffset + p * uPosSize; }
)" +

		fullscreenVs,
		deltaCode +
		std::string(R"(
        in vec2 fsUv;
        out vec4 FragColor;

        uniform sampler2D uwTex;
	    uniform vec2 uSampleOffset;
	    uniform float uScale;

		void main()
		{
       //   if(uScale > 0.9) {

          FragColor = uScale * texture(uwTex, fsUv + uSampleOffset);
     //     } else {
       //      FragColor = vec4( fsUv + uSampleOffset, 0.0, 1.0);
     //      }
		}
		)")
	);
	bsPosOffsetLocation = glGetUniformLocation(boundaryShader, "uPosOffset");
	bsPosSizeLocation = glGetUniformLocation(boundaryShader, "uPosSize");
	bswTexSizeLocation = glGetUniformLocation(boundaryShader, "uwTex");
	bsSampleOffsetLocation = glGetUniformLocation(boundaryShader, "uSampleOffset");
	bsScaleLocation = glGetUniformLocation(boundaryShader, "uScale");

	visShader = LoadNormalShader(
		
		std::string(R"(
       layout(location = 0) in vec3 vsPos;

        out vec2 fsUv;

        void main() {
          fsUv = vsPos.xy;
          gl_Position =  vec4(2.0 * vsPos.xy - vec2(1.0), 0.0, 1.0);
        }
		)"),

		std::string(R"(

        in vec2 fsUv;

        uniform sampler2D uTex;

        out vec4 FragColor;

		void main()
		{
          FragColor = vec4(texture2D(uTex, fsUv).rgb, 1.0);
		}
		)")
	);
	vsTexLocation = glGetUniformLocation(visShader, "uTex");
	
	{
		std::vector<FullscreenVertex> vertices;

		vertices.push_back(FullscreenVertex{ +0.0f, +0.0f });
		vertices.push_back(FullscreenVertex{ +1.0f, +0.0f });
		vertices.push_back(FullscreenVertex{ +0.0f, +1.0f });

		vertices.push_back(FullscreenVertex{ +1.0f, +0.0f });
		vertices.push_back(FullscreenVertex{ +1.0f, +1.0f });
		vertices.push_back(FullscreenVertex{ +0.0f, +1.0f });

		// upload geometry to GPU.
		GL_C(glGenBuffers(1, &fullscreenVertexVbo));
		GL_C(glBindBuffer(GL_ARRAY_BUFFER, fullscreenVertexVbo));
		GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(FullscreenVertex)*vertices.size(), (float*)vertices.data(), GL_STATIC_DRAW));
	}

	
	LOGI("start loading extension!\n");

	LOGI("Init opengl\n");
}

