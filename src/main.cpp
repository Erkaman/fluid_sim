#include <cstdlib>

#include <cstring>
#include <string>

#include <vector>
#include <string>

#include <math.h>

#include <chrono>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "stb_image.h"

#include <glad/glad.h>
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#include <GLFW/glfw3.h>

inline void checkOpenGLError(const char* stmt, const char* fname, int line)
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
	checkOpenGLError(#stmt, __FILE__, __LINE__);	\
    } while (0)
#endif

// define this one, if you need it for debugging.
#undef DEBUG_GROUPS

inline char* getShaderLogInfo(GLuint shader) {
	GLint len;
	GLsizei actualLen;
	GL_C(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
	char* infoLog = new char[len];
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));
	return infoLog;
}

inline GLuint createShaderFromString(const std::string& shaderSource, const GLenum shaderType) {
	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));
	const char *c_str = shaderSource.c_str();
	GL_C(glShaderSource(shader, 1, &c_str, NULL));
	GL_C(glCompileShader(shader));

	GLint compileStatus;
	GL_C(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
	if (compileStatus != GL_TRUE) {
		printf("Could not compile shader\n\n%s \n\n%s\n", shaderSource.c_str(),
			getShaderLogInfo(shader));
		exit(1);
	}

	return shader;
}

inline GLuint loadNormalShader(const std::string& vsSource, const std::string& fsShader) {

	std::string prefix = "";
	prefix = "#version 330\n";

	GLuint vs = createShaderFromString(prefix + vsSource, GL_VERTEX_SHADER);
	GLuint fs = createShaderFromString(prefix + fsShader, GL_FRAGMENT_SHADER);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint Result;
	glGetProgramiv(shader, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		printf("Could not link shader \n\n%s\n", getShaderLogInfo(shader));
		exit(1);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
}

GLFWwindow* window;

const int WINDOW_WIDTH = 256 * 4;
const int WINDOW_HEIGHT = 256 * 4;

GLuint vao;

int fbWidth, fbHeight;

struct FullscreenVertex {
	float x, y; // position
};
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
GLuint fsCounterLocation;

GLuint addColorShader;
GLuint accTexLocation;
GLuint acCounterLocation;

GLuint writeTexShader;
GLuint wtcTexLocation;
GLuint wtOffsetLocation;
GLuint wtSizeLocation;

GLuint fbo0;

// velocity tex.
GLuint uBegTex;
GLuint uEndTex;

// color tex
GLuint cBegTex;
GLuint cEndTex;
GLuint cTempTex;

GLuint wTex;
GLuint wTempTex;
GLuint wDivergenceTex;
GLuint pTempTex[2];
GLuint pTex;
GLuint uEndTempTex;

GLuint monaTex;
GLuint screamTex;

void initGlfw() {
	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Flashy Fluid Simulation Demo", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	glfwSetWindowPos(window, 20, 20);

	// load GLAD.
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Bind and create VAO, otherwise, we can't do anything in OpenGL.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
}

// render fullscren quad.
void renderFullscreen() {
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

// compute divergence of src, put the result in dst. 
void computeDivergence(GLuint src, GLuint dst) {
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

			renderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
}

// advect src, using u as velocity, and put the result into dst.
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

		renderFullscreen();
	}
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// solve the poisson pressure equation, by simple jacobi iteration.
// that is, solve for x in
// (nabla^2)(x) = b.
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

			GL_C(glUniform2f(jsAlphaLocation, -1.0f, -1.0f));
			GL_C(glUniform2f(jsBetaLocation, 1.0 / 4.0, 1.0 / 4.0));


			renderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	
	// now we return the calculuated pressure:
	return tempTex[(iter + 0) % 2];
}

// these two are pretty useful, when debugging in RenderDoc or Nsight for instance.
void dpush(const char* str) {
#ifdef DEBUG_GROUPS
	glad_glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, str);
#endif
}
void dpop() {
#ifdef DEBUG_GROUPS
	glad_glPopDebugGroup();
#endif
}


void renderFrame() {
	// setup some reasonable default GL state.
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

	GL_C(glViewport(0, 0, fbWidth, fbHeight));
	
	// enable vertex buffer used for full screen quad rendering. 
	// this buffer is used for all rendering, from now on.
	GL_C(glEnableVertexAttribArray((GLuint)0));
	GL_C(glBindBuffer(GL_ARRAY_BUFFER, fullscreenVertexVbo));
	GL_C(glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, sizeof(FullscreenVertex), (void*)0));

	dpush("color Advection");
	advect(cBegTex, uBegTex, cTempTex);
	dpop();
	
	dpush("velocity Advection");
	advect(uBegTex, uBegTex, wTex);
	dpop();

	// we use this simple counter for progressing the state of the the simulation.
	static int icounter = 0;
	icounter++;
	float counter = icounter * 1.0f;

	// add force.
	dpush("c Add Force");
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

			GL_C(glUniform1f(fsCounterLocation, float(counter)));

			renderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	dpop();

	// add color.
	dpush("Add Color");
	{
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			cEndTex,
			//uEndTex,	
			0));
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
		{
			GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
			GL_C(glClear(GL_COLOR_BUFFER_BIT));

			GL_C(glUseProgram(addColorShader));

			GL_C(glUniform1i(accTexLocation, 0));
			GL_C(glActiveTexture(GL_TEXTURE0 + 0));
			GL_C(glBindTexture(GL_TEXTURE_2D, cTempTex));

			GL_C(glUniform1f(acCounterLocation, float(counter)));

			renderFullscreen();
		}
		GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	dpop();

	if (counter == 1 && false) {
		// add color.
		dpush("Write Mona Lisa");
		{
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				//cBegTex,
				cEndTex,
				0));
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			{
				GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
				GL_C(glClear(GL_COLOR_BUFFER_BIT));

				GL_C(glUseProgram(writeTexShader));

				GL_C(glUniform1i(wtcTexLocation, 0));
				GL_C(glActiveTexture(GL_TEXTURE0 + 0));
				GL_C(glBindTexture(GL_TEXTURE_2D, monaTex));

				GL_C(glUniform2f(wtOffsetLocation, -1.0f, -1.0f));
				GL_C(glUniform2f(wtSizeLocation, +2.0f, +2.0f));

				renderFullscreen();
			}
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		dpop();
	}

	if (counter == 1 && false) {
		// add color.

		dpush("Write Scream");
		{
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				//cBegTex,
				cEndTex,
				0));
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));

			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, fbo0));
			{
				GL_C(glUseProgram(writeTexShader));

				GL_C(glUniform1i(wtcTexLocation, 0));
				GL_C(glActiveTexture(GL_TEXTURE0 + 0));
				GL_C(glBindTexture(GL_TEXTURE_2D, screamTex));

				GL_C(glUniform2f(wtOffsetLocation, -1.0f, -1.0f));
				GL_C(glUniform2f(wtSizeLocation, +2.0f, +2.0f));

				renderFullscreen();
			}
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		dpop();
	}

	dpush("Pressure Gradient Subtract");
	// subtraction of pressure gradient.
	{
		dpush("Compute divergence of w");
		computeDivergence(wTempTex, wDivergenceTex);
		dpop();

		dpush("Compute pressure");
		// compute pressure.
		{

			dpush("Clear pTemp Textures");
			clearTexture(pTempTex[0]);
			clearTexture(pTempTex[1]);
			dpop();

			dpush("Jacobi");

			pTex = jacobi(40,
				wDivergenceTex, // b
				pTempTex
			);

			dpop();

		}
		dpop();

		//	I think maybe the texture sampling coordinates are wrong. try not samplignthe texel center.

		dpush("pressure gradient subtraction");
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

				renderFullscreen();
			}
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		dpop();

	}
	dpop();

	dpush("Rendering");
	{
		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT));

		GL_C(glUseProgram(visShader));

		GL_C(glUniform1i(vsTexLocation, 0));
		GL_C(glActiveTexture(GL_TEXTURE0 + 0));
		GL_C(glBindTexture(GL_TEXTURE_2D, cEndTex));

		renderFullscreen();
	}
	dpop();

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

void handleInput() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

GLuint createFloatTexture(float* data, GLint internalFormat, GLint format, GLenum type) {
	GLuint tex;

	GL_C(glGenTextures(1, &tex));
	GL_C(glBindTexture(GL_TEXTURE_2D, tex));
	GL_C(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, fbWidth, fbHeight, 0, format, type, data));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_C(glBindTexture(GL_TEXTURE_2D, 0));
	
	return tex;
}

GLuint loadJpgAsTexture(const char* filepath) {
	FILE* fh = fopen(filepath, "rb");
	if (fh == nullptr) {
		printf("COULD NOT OPEN %s. Make sure it is in path\n", filepath);
	}
	
	int width;
	int height;
	int depth;
	GLubyte* bitmap = static_cast<GLubyte*>(stbi_load_from_file(fh, &width, &height,
		&depth,
		STBI_default));

	GLint format = 0, internalFormat = 0;
	
	switch (depth) {

	case STBI_rgb: {
		format = GL_RGB;
		internalFormat = GL_RGB;
		break;
	}
	case STBI_rgb_alpha: {
		format = GL_RGBA;
		internalFormat = GL_RGBA;

		break;
	}

	}
	
	GLuint tex;

	GL_C(glGenTextures(1, &tex));
	GL_C(glBindTexture(GL_TEXTURE_2D, tex));
	GL_C(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, bitmap));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	return tex;
}

void setupGraphics() {
	initGlfw();

	// create all textures.
	{
		float* zeroData = new float[fbWidth * fbHeight * 4];
		for (int y = 0; y < fbHeight; ++y) {
			for (int x = 0; x < fbWidth; ++x) {
				zeroData[4 * (fbWidth * y + x) + 0] = 0.0f;
				zeroData[4 * (fbWidth * y + x) + 1] = 0.0f;
				zeroData[4 * (fbWidth * y + x) + 2] = 0.0f;
				zeroData[4 * (fbWidth * y + x) + 3] = 0.0f;
			}
		}
		
		// note that we use RG32F to store the velocity fields. This is actually very important.
		// since fluid simulation is heavily bandwidth bound, using RG32F instead of RGBA32F essentially doubled the performance
		// in our measurments.
		cBegTex = createFloatTexture(zeroData, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		uBegTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		uEndTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		cEndTex = createFloatTexture(zeroData, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		cTempTex = createFloatTexture(zeroData, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		wTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		wTempTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		wDivergenceTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		uEndTempTex = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		pTempTex[0] = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);
		pTempTex[1] = createFloatTexture(zeroData, GL_RG32F, GL_RG, GL_FLOAT);

		monaTex = loadJpgAsTexture("../smallmona.jpg");
		screamTex = loadJpgAsTexture("../smallscream.jpg");
	}
	
	GL_C(glGenFramebuffers(1, &fbo0));

	std::string fullscreenVs(R"(
       layout(location = 0) in vec3 vsPos;

        out vec2 fsUv;

        void main() {
          fsUv = vsPos.xy;
          gl_Position =  vec4(2.0 * vsPos.xy - vec2(1.0), 0.0, 1.0);
        }
		)");

	std::string deltaCode = std::string("const vec2 delta = vec2(") + std::to_string(1.0f / float(fbWidth)) + std::string(",") + std::to_string(1.0f / float(fbHeight)) + ");\n";
	
	std::string fragDefines = "";
	fragDefines += deltaCode;

	//std::string remapUvCode = std::string("vec2 remapUv(vec2 p) { return 1.5*delta + p * (1.0 - 3.0 * delta); }\n");

	std::string vertDefines = "";
	vertDefines += deltaCode;
	
	advectShader = loadNormalShader(
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
          vec2 tc = fsUv - delta * (1.0 / 60.0) * texture(uuTex, fsUv).xy;
          FragColor = texture(usTex, tc);

		}
		)")
	);
	asuTexLocation = glGetUniformLocation(advectShader, "uuTex");
	assTexLocation = glGetUniformLocation(advectShader, "usTex");

	jacobiShader = loadNormalShader(
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

          vec4 bC = vec4(texture(ubTex, fsUv).x);

          FragColor = (xR + xL + xT + xB + vec4(uAlpha.xy, 0.0, 0.0) * bC) * vec4(uBeta.xy, 0.0, 0.0);
		}
		)")
	);
	jsxTexLocation = glGetUniformLocation(jacobiShader, "uxTex");
	jsbTexLocation = glGetUniformLocation(jacobiShader, "ubTex");
	jsBetaLocation = glGetUniformLocation(jacobiShader, "uBeta");
	jsAlphaLocation = glGetUniformLocation(jacobiShader, "uAlpha");

	divergenceShader = loadNormalShader(
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

	gradientSubtractionShader = loadNormalShader(
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

	std::string emitterCode = std::string(R"(

vec2 F;
vec3 C;
in vec2 fsUv;

uniform float uCounter;


vec2 uForce;
vec2 uPos;
vec3 uColor;
float uRad;


float hash(float n)
{
    return fract(sin(n)*43758.5453123);
}

float mynoise(in vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);

    f = f*f*(3.0-2.0*f);

    float n = p.x + p.y*57.0;
    float res = mix(mix(hash(n+  0.0), hash(n+  1.0), f.x),
                    mix(hash(n+ 57.0), hash(n+ 58.0), f.x), f.y);
    return res;
}

vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

float quarticIn(float t) {
  return pow(t, 4.0);
}

vec3 colorize(float t, vec2 uv) {
  float p = 0.2;
  vec3 col = vec3(0.0, 0.0, 0.0);

  t += float(uCounter) / 200;
  col = 1.2 * pal( t, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.33,0.67) );

 return col;
}

void rainbowEmit() {
  float t = 2.0f * float(uCounter) / 500.0f;

  float b;
  b = 0.0 * 3.14 + 0.35f * sin(40.0f * t + 5.4 * hash(float(uCounter)/300.0)  );
  uForce= vec2(9.2 * 60.0f * sin(b), 9.2 * 60.0 *  cos(b));
  uPos = vec2(0.5 + 0.05*sin(float(uCounter)/10.0), 0.1);
  uColor = vec3(0.5f, 0.0, 0.0);
  uRad = 0.02f;
  
  float dist = distance(fsUv, uPos);
  t = max(uRad - dist, 0.0)/uRad;

  F +=  (t) * uForce;
  if(uRad - dist > 0.0) C = colorize(t, fsUv);
}

// idea, do this emitter in a circle.
void emit(vec2 eDir, vec2 ePos, vec3 pa, vec3 pb, vec3 pc, vec3 pd) {

  uRad = 0.005f;
  uColor = vec3(1.0, 0.0, 0.0);
  uPos = ePos+ eDir * float(uCounter) / 500.0f;
  uForce = eDir * 70.0 + 100.0 * (-1.0 + 2.0* mynoise(300.0 *  uPos));

  float dist = distance(fsUv, uPos);
  float t = max(uRad - dist, 0.0)/uRad;

  F +=  (t) * uForce;
  
{
  float p = 0.2;
  vec3 col = vec3(0.0, 0.0, 0.0);
  float tt = t;
  tt += float(uCounter) / 200;

  col = 0.6 * pal( 1.0 * tt, pa, pb, pc, pd );

  if(uRad - dist > 0.0) C += col;

  dist = distance(fsUv,  ePos - 0.1 * eDir + eDir * (float(uCounter) / 500.0f)  );
  t = max(uRad - dist, 0.0)/uRad;
  float theta = 0.0f + 3.14 * 2.0 * mynoise(300.0 *  uPos);
  F +=  (t) * 70.0 * vec2(cos(theta), sin(theta));
}

}

void circleEmitter() {

  int N = 14;
  for(int i = 0; i < N; ++i) {
    float theta = 2.0 * 3.14 *  i / float(N);
    vec2 pos = vec2(0.5, 0.5) + 0.3 * vec2(cos(theta) , sin(theta));
    vec2 dir = -vec2(cos(theta) , sin(theta));

    int j = i % 6;
    if(j == 0) {
      emit(dir, pos, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.33,0.67));
    } else if(j == 1) {
      emit(dir, pos, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.2,0.3,1.0),vec3(0.4,0.33,0.27));
    } else if(j == 2) {
      emit(dir, pos,vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(0.4,0.3,0.3),vec3(0.8,0.9,0.28));
    }else if(j == 3) {
      emit(dir, pos,  vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.3,0.7,0.4),vec3(3.46,0.8,0.17));
    } else if(j == 4) {
      emit(dir, pos,  vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(0.3,0.2,1.2),vec3(1.46,1.1,0.57));
    }else if(j == 5) {
      emit(dir, pos,  vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(0.8,1.1,0.7),vec3(0.15,0.1,0.03));
    }
  }
}

void goghEmitter(vec2 ePos, vec2 eDir, float myc) {
   

  uRad = 0.002f;
  uColor = vec3(1.0, 0.0, 0.0);
  uPos = ePos+ eDir * float(myc) / 500.0f;
  uForce = eDir * 70.0 + 100.0 * (-1.0 + 2.0* mynoise(300.0 *  uPos));

  float dist = distance(fsUv, uPos);
  float t = max(uRad - dist, 0.0)/uRad;

  F +=  (t) * uForce;

{
  float p = 0.2;
  float tt = t;
  tt += float(myc) / 200;
 
  dist = distance(fsUv,  ePos - 0.1 * eDir + eDir * (float(myc) / 500.0f)  );
  t = max(uRad - dist, 0.0)/uRad;
  float theta = 0.0f + 3.14 * 2.0 * mynoise(300.0 *  uPos);
  F +=  (t) * 1.0 * vec2(cos(theta), sin(theta));
}

}

void emitter() {
  //rainbowEmit();

 circleEmitter();

// single mona lisa that covers screen.
/*
if(uCounter > 3) {
 
  uRad = 0.005f;
  uColor = vec3(1.0, 0.0, 0.0);
  vec2 ePos = vec2(0.1, 0.5);
  vec2 eDir = normalize(vec2(0.5, 0.5));
  for(float x = 0.02; x < 0.98; x += 0.05) {
    for(float y = 0.02; y < 0.98; y += 0.05) {
      uPos = vec2(x, y);
      float theta = 0.0f + 3.14 * 2.0 * mynoise(300.0 *  uPos + vec2(uCounter / 200.0) );
      uForce = 1.0 * vec2(cos(theta), sin(theta));
      float dist = distance(fsUv, uPos);
      float t = max(uRad - dist, 0.0)/uRad;
      F +=  (t) * uForce;
    }
  }
}
*/

/*
if(uCounter > 3) {
 goghEmitter(vec2(0.5, 0.5), normalize(vec2(0.4, 0.8)), uCounter-3);   
}
if(uCounter > 60) {
 goghEmitter(vec2(0.3, 0.3), normalize(vec2(-0.2, 0.4)), uCounter-60);   
}
if(uCounter > 90) {
 goghEmitter(vec2(+0.8, 0.2), normalize(vec2(-0.4, +0.4)), uCounter-90);   
}
if(uCounter > 130) {
 goghEmitter(vec2(+0.5, 0.96), normalize(vec2(0.1, -1.0)), uCounter-130);   
}
if(uCounter > 160) {
 goghEmitter(vec2(+0.1, 0.1), normalize(vec2(0.3, +0.08)), uCounter-160);   
}

if(uCounter > 200) {
 goghEmitter(vec2(+0.19, 0.98), normalize(vec2(0.1, -0.4)), uCounter-200);   
}
if(uCounter > 250) {
 goghEmitter(vec2(+0.01, 0.01), normalize(vec2(0.1, 0.1)), uCounter-250);   
}
if(uCounter > 300) {
 goghEmitter(vec2(+0.8, 0.1), normalize(vec2(-0.1, 0.8)), uCounter-300);   
}

if(uCounter > 320) {
 goghEmitter(vec2(+0.2, 0.9), normalize(vec2(0.0, -0.1)), uCounter-320);   
}

if(uCounter > 330) {
 goghEmitter(vec2(+0.5, 0.5), normalize(vec2(-0.6, 0.0)), uCounter-330);   
}

if(uCounter > 350) {
 goghEmitter(vec2(+0.1, 0.8), normalize(vec2(1.0, 0.0)), uCounter-350);   
}
if(uCounter > 360) {
 goghEmitter(vec2(+0.1, 0.1), normalize(vec2(1.0, 0.0)), uCounter-360);   
}
if(uCounter > 380) {
 goghEmitter(vec2(+0.9, 0.9), normalize(vec2(-1.0, 0.0)), uCounter-380);   
}

if(uCounter > 400) {
 goghEmitter(vec2(+0.5, 0.04), normalize(vec2(0.0, 0.9)), uCounter-400);   
}
if(uCounter > 420) {
 goghEmitter(vec2(+0.89, 0.9), normalize(vec2(0.0, -0.9)), uCounter-420);   
}

if(uCounter > 440) {
 goghEmitter(vec2(+0.11, 0.1), normalize(vec2(0.0, +0.9)), uCounter-440);   
}
*/

}


)");

	forceShader = loadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		emitterCode +
		std::string(R"(


        uniform sampler2D uwTex;

        out vec4 FragColor;

		void main()
		{
          F = vec2(0.0, 0.0);

          emitter();



          FragColor = vec4(F.xy, 0.0, 0.0) + texture(uwTex, fsUv);
		}
		)")
	);
	fswTexLocation = glGetUniformLocation(forceShader, "uwTex");
	fsCounterLocation = glGetUniformLocation(forceShader, "uCounter");

	addColorShader = loadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		emitterCode +
		std::string(R"(

        uniform sampler2D ucTex;

        out vec4 FragColor;

		void main()
		{
          C = vec3(0.0, 0.0, 0.0);

          emitter();
          FragColor = vec4(C.rgb, 0.0) + texture(ucTex, fsUv);
		}
		)")
	);
	accTexLocation = glGetUniformLocation(addColorShader, "ucTex");
	acCounterLocation = glGetUniformLocation(addColorShader, "uCounter");

	writeTexShader = loadNormalShader(
		//vertDefines +

		R"(
		layout(location = 0) in vec3 vsPos;

	out vec2 fsUv;

    uniform vec2 uOffset;
    uniform vec2 uSize;


	void main() {
		fsUv = (vsPos.xy);
//		gl_Position = vec4(2.0 * (vsPos.xy) - vec2(1.0), 0.0, 1.0);
		gl_Position = vec4(uSize * (vsPos.xy) + uOffset, 0.0, 1.0);
	}
	)",

		fragDefines +
		std::string(R"(

        uniform sampler2D ucTex;

        out vec4 FragColor;
        in vec2 fsUv;

		void main()
		{
          vec3 c = texture(ucTex, vec2(fsUv.x, 1.0 - fsUv.y) ).rgb;
          if(length(c) < 0.1) {
            discard;
          }

          FragColor = vec4(pow(c, vec3(2.2)), 1.0);

       //   FragColor = vec4(1.0, 0.0, 0.0, 1.0);

		}
		)")
	);
	wtcTexLocation = glGetUniformLocation(writeTexShader, "ucTex");
	wtOffsetLocation = glGetUniformLocation(writeTexShader, "uOffset");
	wtSizeLocation = glGetUniformLocation(writeTexShader, "uSize");

	visShader = loadNormalShader(

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
          FragColor = vec4(pow(texture2D(uTex, fsUv).rgb, vec3(1.0 / 2.2)), 1.0);

		}
		)")
	);
	vsTexLocation = glGetUniformLocation(visShader, "uTex");

	// create vertices of fullscreen quad.
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
}

int main(int argc, char** argv) {
	setupGraphics();

	float frameStartTime = 0;
	float frameEndTime = 0;
	frameStartTime = (float)glfwGetTime();
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		handleInput();
		renderFrame();

		glfwSwapBuffers(window);
		
		// FPS regulation code. we will ensure that a framerate of 30FPS is maintained.
		// and for simplicity, we just assume that the computer is always able to maintain a framerate of at least 30FPS.
		{
			frameEndTime = (float)glfwGetTime();
			float frameDuration = frameEndTime - frameStartTime;
			const float sleepDuration = 1.0f / 30.0f - frameDuration;
			if (sleepDuration > 0.0f) {
				std::this_thread::sleep_for(std::chrono::milliseconds((int)(sleepDuration  * 1000.0f)));
			}
			frameStartTime = (float)glfwGetTime();
		}
	}

	glfwTerminate();
	exit(EXIT_SUCCESS);
}