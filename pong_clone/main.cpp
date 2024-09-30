#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

// source: https://red_paddlenoiro.jp/
constexpr char RED_PADDLE_SPRITE_FILEPATH[] = "red_paddle.png",
BLUE_PADDLE_SPRITE_FILEPATH[] = "blue_paddle.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(0.25f, 0.75595f, 0.0f),
INIT_POS_RED_PADDLE = glm::vec3(-4.0f, 0.0f, 0.0f),
INIT_POS_BLUE_PADDLE = glm::vec3(4.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_red_paddle_matrix,
g_blue_paddle_matrix,
g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_red_paddle = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_blue_paddle = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_red_paddle_texture_id,
g_blue_paddle_texture_id;

glm::vec3 g_red_paddle_position = glm::vec3(0.0f, 0.0f, 0.0f),
g_red_paddle_movement = glm::vec3(0.0f, 0.0f, 0.0f),
g_blue_paddle_position = glm::vec3(0.0f, 0.0f, 0.0f),
g_blue_paddle_movement = glm::vec3(0.0f, 0.0f, 0.0f);


constexpr float g_paddle_speed = 1.0f;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_red_paddle_matrix = glm::mat4(1.0f);
    g_blue_paddle_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_red_paddle_texture_id = load_texture(RED_PADDLE_SPRITE_FILEPATH);
    g_blue_paddle_texture_id = load_texture(BLUE_PADDLE_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_w:
                        // Move the player left
                        g_red_paddle_movement.y = 1.0f;
                        break;

                    case SDLK_s:
                        // Move the player right
                        g_red_paddle_movement.y = -1.0f;
                        break;

                    case SDLK_UP:
                        g_blue_paddle_movement.y = 1.0f;
                        break;

                    case SDLK_DOWN:
                        g_blue_paddle_movement.y = -1.0f;
                        break;

                    default:
                        break;
                }

        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_W])
    {
        g_red_paddle_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {   
        g_red_paddle_movement.y = -1.0f;
    }

    if (key_state[SDL_SCANCODE_UP])
    {
        g_blue_paddle_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_blue_paddle_movement.y = -1.0f;
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
    g_rotation_red_paddle.y += ROT_INCREMENT * delta_time;
    g_rotation_blue_paddle.y += -1 * ROT_INCREMENT * delta_time;

    /* Model matrix reset */
    g_red_paddle_matrix = glm::mat4(1.0f);
    g_blue_paddle_matrix = glm::mat4(1.0f);

    /* Transformations */
    /*
    g_red_paddle_matrix = glm::rotate(g_red_paddle_matrix,
        g_rotation_red_paddle.y,
        glm::vec3(0.0f, 1.0f, 0.0f));

    g_blue_paddle_matrix = glm::rotate(g_blue_paddle_matrix,
        g_rotation_blue_paddle.y,
        glm::vec3(0.0f, 1.0f, 0.0f));*/
    g_red_paddle_position += g_red_paddle_movement * g_paddle_speed * delta_time;
    g_red_paddle_matrix = glm::translate(g_red_paddle_matrix, INIT_POS_RED_PADDLE);
    g_red_paddle_matrix = glm::translate(g_red_paddle_matrix, g_red_paddle_position);
    
    g_blue_paddle_position += g_blue_paddle_movement * g_paddle_speed * delta_time;
    g_blue_paddle_matrix = glm::translate(g_blue_paddle_matrix, INIT_POS_BLUE_PADDLE);
    g_blue_paddle_matrix = glm::translate(g_blue_paddle_matrix, g_blue_paddle_position);

    g_red_paddle_matrix = glm::scale(g_red_paddle_matrix, INIT_SCALE);
    g_blue_paddle_matrix = glm::scale(g_blue_paddle_matrix, INIT_SCALE);
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_red_paddle_matrix, g_red_paddle_texture_id);
    draw_object(g_blue_paddle_matrix, g_blue_paddle_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
