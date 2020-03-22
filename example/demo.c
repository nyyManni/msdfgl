
#include <stdio.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#include <msdfgl.h>


void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

GLfloat proj[4][4];

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec4 vertex;\n"
                                 "out vec2 text_pos;\n"
                                 "void main() {\n"
                                 "gl_Position = vec4(vertex.xy, 0.0, 1.0);\n"
                                 "text_pos = vertex.zw;\n"
                                 "}\n";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "precision mediump float;\n"
                                   "in vec2 text_pos;\n"
                                   "uniform sampler2D tex;\n"
                                   "out vec4 color;\n"
                                   "void main() {\n"
                                   "color = texture(tex, text_pos);\n"
                                   "}\n";

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage msdfgldemo <font file> '<text>'\n");
        return -1;
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
                   GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    GLFWwindow *window =
        glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MSDFGL Demo", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s\n", infoLog);
    }
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s\n", infoLog);
    }
    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n %s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint texture_uniform = glGetUniformLocation(shaderProgram, "tex");

    msdfgl_context_t ctx = msdfgl_create_context("330 core");

    if (!ctx) {
        fprintf(stderr, "Failed to create context!\n");
        return -1;
    }
    msdfgl_atlas_t atlas = msdfgl_create_atlas(ctx, 1024, 2);

    msdfgl_font_t font = msdfgl_load_font(ctx, argv[1], 4.0, 2.0, atlas);
    if (!font) {
        fprintf(stderr, "Failed to load font!\n");
        return -1;
    }
    msdfgl_set_missing_glyph_callback(ctx, msdfgl_generate_glyph, NULL);

    _msdfgl_ortho(0.0, SCR_WIDTH, SCR_HEIGHT, 0.0, -1.0, 1.0, proj);
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    GLuint _vbo;
    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLfloat rect[6][4] = {{0, -0.8, 0, 1}, {0, 1, 0, 0},    {1, -0.8, 1, 1},
                          {0, 1, 0, 0},    {1, -0.8, 1, 1}, {1, 1, 1, 0}};

    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), (GLfloat *)rect, GL_DYNAMIC_DRAW);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float y = 200.0;
        for (int i = 2; i < argc; i++) {
            msdfgl_printf(10.0, y, font, 64.0, 0xffffffff, (GLfloat *)proj,
                          MSDFGL_UTF8 | MSDFGL_KERNING, argv[i]);
            y += msdfgl_vertical_advance(font, 64.0);
        }

        glUseProgram(shaderProgram);

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, _msdfgl_atlas_texture(font));
        glUniform1i(texture_uniform, 8);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    msdfgl_destroy_font(font);
    msdfgl_destroy_atlas(atlas);
    msdfgl_destroy_context(ctx);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    _msdfgl_ortho(0.0, (float)width, (float)height, 0.0, -1.0, 1.0, proj);
}
