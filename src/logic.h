
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

PFNGLGENQUERIESPROC glGenQueriesEXT;

PFNGLENDQUERYPROC glEndQueryEXT;
PFNGLBEGINQUERYPROC glBeginQueryEXT;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuivEXT;

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)


std::chrono::time_point<std::chrono::system_clock>  m_frameStartTime;


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


const int WINDOW_WIDTH = 256;
const int WINDOW_HEIGHT = 256;

GLuint vao;

int FRAME_RATE = 30;
int fbWidth, fbHeight;

int frameW, frameH;
int downsampleW, downsampleH;

struct FullscreenVertex {
	float x, y; // position
};


GLuint fullscreenVertexVbo;

GLuint blitShader;
GLuint bsTexLocation;

GLuint visShader;
GLuint vsTexLocation;


GLuint frameFbo;
GLuint frameColorTexture;

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

	// load GLAD.
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Bind and create VAO, otherwise, we can't do anything in OpenGL.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
}
#endif

static const int BUFFER_SIZE = 4;
static GLuint queries[BUFFER_SIZE];
static int query_buffer_next_index;
static int query_buffer_n;
static float elapsed_time_seconds = NAN;

static void InitRenderTiming() {
	GL_C(glGenQueriesEXT(BUFFER_SIZE, queries));

#ifdef WIN32
#else
	GLint disjointOccurred;
	glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjointOccurred);  // clear
#endif
}

static void StartTimingEvent() {

	// on disjoint exception, clear query buffer
#ifdef WIN32
#else
	GLint disjointOccurred = false;
	glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjointOccurred);
	if (disjointOccurred) {
		query_buffer_n = 0;
		LOGI("disjoint exception\n");
	}
#endif

	// read result of oldest pending query
	// We use buffering to avoid blocking when reading results.
	elapsed_time_seconds = NAN;
	if (query_buffer_n > 0) {
		int index =
			(query_buffer_next_index + (BUFFER_SIZE - query_buffer_n)) % BUFFER_SIZE;
		GLuint available = 0;

		GLenum pname;
#ifdef WIN32
		pname = GL_QUERY_RESULT_AVAILABLE;
#else
		pname = GL_QUERY_RESULT_AVAILABLE_EXT;
#endif

		glGetQueryObjectuivEXT(queries[index], pname, &available);
		if (available) {
			GLuint elapsed_time_ns;

#ifdef WIN32
			pname = GL_QUERY_RESULT;
#else
			pname = GL_QUERY_RESULT_EXT;
#endif

			glGetQueryObjectuivEXT(queries[index], pname, &elapsed_time_ns);
			if (glGetError() == GL_NO_ERROR) {
				elapsed_time_seconds = elapsed_time_ns / 1e9f;
			}
			else {
				LOGI("error reading query result\n");
			}
			--query_buffer_n;
		}
	}

	// start new query
	if (query_buffer_n < BUFFER_SIZE) {
		glBeginQueryEXT(GL_TIME_ELAPSED_EXT, queries[query_buffer_next_index]);
		if (glGetError() == GL_NO_ERROR) {
			query_buffer_next_index = (query_buffer_next_index + 1) % BUFFER_SIZE;
			++query_buffer_n;
		}
		else {
			LOGI("glBeginQuery error\n");
		}
	}
	else {
		LOGI("query buffer overflow\n");
	}
}

// End timing query.
// Arg eventID is unused.
// Must be called once for every StartTimingEvent.
static void EndTimingEvent() {
	glEndQueryEXT(GL_TIME_ELAPSED_EXT);
	if (glGetError() != GL_NO_ERROR) LOGI("glEndQuery error\n");
}

// Returns timing period in seconds if available, else NaN.  Reflects the value
// from several frames prior, depending on the level of GPU buffering.
extern "C" float GetTiming() {
	return elapsed_time_seconds;
}

void RenderFullscreen() {
	GL_C(glEnableVertexAttribArray((GLuint)0));
	GL_C(glBindBuffer(GL_ARRAY_BUFFER, fullscreenVertexVbo));
	GL_C(glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, sizeof(FullscreenVertex), (void*)0));

	GL_C(glDrawArrays(GL_TRIANGLES, 0, 6));
}

void renderFrame() {

	// StartTimingEvent();

	static bool firstFrame = true;
	// setup matrices.

	//profiler->Begin(GpuTimeStamp::RENDER_EVERYTHING);



	// setup GL state.
	GL_C(glEnable(GL_DEPTH_TEST));
	GL_C(glDepthMask(true));
	GL_C(glDisable(GL_BLEND));
	GL_C(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
	GL_C(glEnable(GL_CULL_FACE));
	GL_C(glFrontFace(GL_CCW));
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_C(glUseProgram(0));
	GL_C(glBindTexture(GL_TEXTURE_2D, 0));
	GL_C(glDepthFunc(GL_LESS));


	GL_C(glViewport(0, 0, frameW, frameH));
	
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, frameFbo));
	{

		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		GL_C(glUseProgram(blitShader));

		RenderFullscreen();
	}
	GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	
	{

		GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		GL_C(glUseProgram(visShader));

		GL_C(glUniform1i(vsTexLocation, 0));
		GL_C(glActiveTexture(GL_TEXTURE0 + 0));
		GL_C(glBindTexture(GL_TEXTURE_2D, frameColorTexture));

		RenderFullscreen();
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

void setupGraphics(int w, int h) {

#ifdef WIN32
	InitGlfw();
#else
	fbWidth = w;
	fbHeight = h;
#endif

	frameW = fbWidth;
	frameH = fbHeight;

	downsampleW = fbWidth;
	downsampleH = fbHeight;

	// frameFbo
	{
		{
			GL_C(glGenTextures(1, &frameColorTexture));
			GL_C(glBindTexture(GL_TEXTURE_2D, frameColorTexture));
			GL_C(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameW, frameH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
			GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

			GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GL_C(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

			GL_C(glBindTexture(GL_TEXTURE_2D, 0));
		}


		{

			GL_C(glGenFramebuffers(1, &frameFbo));
			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, frameFbo));

			GL_C(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameColorTexture, 0));

			checkFbo();

			GL_C(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
	}

	std::string fullscreenVs(R"(
       layout(location = 0) in vec3 vsPos;

        out vec2 fsUv;

        void main() {
          fsUv = remap(vsPos.xy);
          gl_Position =  vec4(2.0 * remap(vsPos.xy) - vec2(1.0), 0.0, 1.0);
        }
		)");

	vec2 delta(1.0f / frameW, 1.0f / frameH);

	std::string fragDefines = "";
	fragDefines += std::string("#define delta vec2(") + std::to_string(delta.x) + std::string(",") + std::to_string(delta.y) + ")\n";

	std::string vertDefines = "";
	vertDefines += fragDefines;
	vertDefines += std::string("vec2 remap(vec2 p) { return delta + p * (1.0 - 2.0 * delta); }\n");

	blitShader = LoadNormalShader(
		vertDefines +
		fullscreenVs,
		fragDefines +
		std::string(R"(

        in vec2 fsUv;

      uniform sampler2D uTex;
        //uniform sampler2D uDownsampleTex;

        //uniform vec2 uScreenSpaceLightPos;

#define NUM_SAMPLES 100
        out vec4 FragColor;

		void main()
		{
//          FragColor = vec4(fsUv, 0.0,  1.0);
          FragColor = vec4(1.0, 0.0, 0.0,  1.0);
		}
		)")
	);
	bsTexLocation = glGetUniformLocation(blitShader, "uColorTex");

	
	visShader = LoadNormalShader(
		
		std::string("vec2 remap(vec2 p) { return p; }\n") +

		fullscreenVs,

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

#ifdef WIN32

	glGenQueriesEXT = glGenQueries;

	glEndQueryEXT = glEndQuery;

	glBeginQueryEXT = glBeginQuery;

	glGetQueryObjectuivEXT = glGetQueryObjectuiv;

#else

	glGenQueriesEXT = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");

	glEndQueryEXT = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");

	glBeginQueryEXT = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");

	glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXTPROC)eglGetProcAddress("glGetQueryObjectuivEXT");
	// glGetQueryObjectuiv

#endif

	InitRenderTiming();

	m_frameStartTime = std::chrono::system_clock::now();

}

