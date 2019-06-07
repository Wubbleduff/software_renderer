#include "software_renderer.h"
#include "my_math.h"
#include "asset_loading.h"

#include "..\lib\glew-2.1.0\include\GL\glew.h"
#include "..\lib\glfw-3.2.1.bin.WIN32\include\GLFW\glfw3.h"

#include <stdlib.h> // exit
#include <stdio.h>  // printf
#include <assert.h> // assert
#include <vector>

struct Model
{
  v3 position;
  v3 scale;
  f32 rotation;


  std::vector<v3> vertices;
  std::vector<u32> vertex_indices;
};

struct PixelInfo
{
};

struct RendererData
{
  GLFWwindow *window;
  u32 screen_width;
  u32 screen_height;
  u32 num_pixels;
  f32 aspect_ratio;

  v3 camera_position;
  f32 camera_width;
  bool proj_type; // false is ortho, true is perspective
  f32 near_plane;
  f32 far_plane;

  GLuint pboid; // Framebuffer
  GLuint vaoid;
  GLuint texid;

  f32 *depth_buffer;
  PixelInfo *pixel_info_buffer;

  u32 clear_color;

  Model *model;


  std::vector<v4> vertex_buffer;
  std::vector<u32> index_buffer;

  std::vector<v4> clipped_vertex_buffer;
  std::vector<u32> clipped_index_buffer;
};

struct Color
{
  f32 r, g, b, a;

  Color(f32 red, f32 green, f32 blue, f32 alpha) : r(red), g(green), b(blue), a(alpha)
  {
  }

  Color(f32 red, f32 green, f32 blue) : r(red), g(green), b(blue), a(1.0f)
  {
  }


  u32 pack()
  {
    u32 red = (u32)(r * 255.0f);
    u32 green = (u32)(g * 255.0f);
    u32 blue = (u32)(b * 255.0f);
    u32 alpha = (u32)(a * 255.0f);

    u32 result = (red << 0) | (green << 8) | (blue << 16) | (alpha << 24);
    return result;
  }
};

struct EdgeEquation
{
  f32 a, b, c;
  bool tl;
};

enum ClipPlane
{
  LEFT_CLIP_PLANE,
  RIGHT_CLIP_PLANE,
  BOTTOM_CLIP_PLANE,
  TOP_CLIP_PLANE,
  NEAR_CLIP_PLANE,
  FAR_CLIP_PLANE
};

static RendererData renderer_data;
static bool button_states[512];




static void resize_callback(GLFWwindow *window, s32 width, s32 height)
{
  glViewport(0, 0, width, height);
  renderer_data.screen_width = width;
  renderer_data.screen_height = height;
  renderer_data.num_pixels = width * height;
  renderer_data.aspect_ratio = (f32)width / (f32)height;
}

static void key_callback(GLFWwindow *window, s32 key, s32 scan_code, s32 action, s32 mod)
{
  // Keyboard input
  if(action == GLFW_PRESS)
  {
    if(key == GLFW_KEY_ESCAPE) exit(0);

    button_states[key] = true;
  }
  else if(action == GLFW_REPEAT)
  {
  }
  else if(action == GLFW_RELEASE)
  {
    button_states[key] = false;
  }
}

static void mousebutton_callback(GLFWwindow *window, s32 button, s32 action, s32 mod)
{
  // Mouse button input
  if(action == GLFW_PRESS)
  {
    button_states[GLFW_MOUSE_BUTTON_LEFT] = true;
  }
  else if(action == GLFW_REPEAT)
  {
  }
  else if(action == GLFW_RELEASE)
  {
    button_states[GLFW_MOUSE_BUTTON_LEFT] = false;
  }
}



static EdgeEquation edge_equation(v3 start, v3 end)
{
  EdgeEquation eqn;
  eqn.a = start.y - end.y;
  eqn.b = end.x - start.x;
  eqn.c = start.x * end.y - end.x * start.y;

  eqn.tl = (eqn.a != 0.0) ? (eqn.a > 0.0 ? true : false) : (eqn.b < 0 ? true : false);

  return eqn;
}

static void print_pixel_info(u32 x, u32 y)
{
  PixelInfo *pixel = &renderer_data.pixel_info_buffer[y * renderer_data.screen_width + x];

  printf("\n\n");
}

// Render a triangle in viewport pixel space between points p0, p1, p2 with the given color (depending on the render mode)
// p0, p1, p2 will only show if in counter-clockwise order
// If there are normals and/or texture coords, there must be 3 normals and/or 3 texture coordinates given
static void render_triangle(u32 *pixels, v3 p0, v3 p1, v3 p2, v3 *normals, v2 *texture_coords, v3 centroid)
{
  v3 crossp = cross(p1 - p0, p2 - p0);

  // Backface culling
  if(crossp.z < 0.0f) 
  {
    return;
  }

  // Normalize normals
  if(normals)
  {
    for(u32 i = 0; i < 3; i++) normals[i] = unit(normals[i]);
  }

  f32 *depth_buffer = renderer_data.depth_buffer;

  // Get the bounding box of pixels to check the triangle against
  v2 bottom_left_bound = v2(min(p0.x, p1.x, p2.x), min(p0.y, p1.y, p2.y));
  v2 top_right_bound = v2(max(p0.x, p1.x, p2.x), max(p0.y, p1.y, p2.y));
  u32 left_bb = (u32)bottom_left_bound.x;
  u32 bottom_bb = (u32)bottom_left_bound.y;
  u32 right_bb = (u32)top_right_bound.x;
  u32 top_bb = (u32)top_right_bound.y;

  u32 width = renderer_data.screen_width;
  u32 height = renderer_data.screen_height;
#if 0
  assert(left_bb >= 0);
  assert(bottom_bb >= 0);
  assert(right_bb < width);
  assert(top_bb < height);
#else
  if(left_bb < 0) left_bb = 0;
  if(bottom_bb < 0) bottom_bb = 0;
  if(right_bb >= width - 1) right_bb = width - 1;
  if(top_bb >= height - 1) top_bb = height - 1;
#endif

  // These edge equations come from the equation:
  //
  // p and q are points on the triangle
  // e is the edge from p to q
  // point x is on the line e if
  
  // n * (x - p) = 0
  // n * x - n * p = 0
  //
  // a and b from the edge equation are the x and y components of n * x
  // c is the n * p component
  //
  EdgeEquation e0 = edge_equation(p1, p2);
  EdgeEquation e1 = edge_equation(p2, p0);
  EdgeEquation e2 = edge_equation(p0, p1);


  // Pick the starting pixel
  v3 pixel = {left_bb + 0.5f, bottom_bb + 0.5f, 1.0f};

  // These values represent the evaluations of each edge equation
  // These are initialized here from the beginning pixel so they can be computed incrementally
  f32 eval0 = e0.a * pixel.x + e0.b * pixel.y + e0.c;
  f32 eval1 = e1.a * pixel.x + e1.b * pixel.y + e1.c;
  f32 eval2 = e2.a * pixel.x + e2.b * pixel.y + e2.c;

  // 2 * triangle area is just the some the edge equations
  // Each edge equation evaluation value represents twice the area of the triangle made from
  // the point inside the triangle and the two opposite triangle vertices
  f32 double_triangle_area = eval0 + eval1 + eval2;

  // Initial calculations of barycentric coordinates for interpolation
  f32 a =      eval0 / double_triangle_area;
  f32 a_inc_x = e0.a / double_triangle_area;
  f32 a_inc_y = e0.b / double_triangle_area;
  f32 b =      eval1 / double_triangle_area;
  f32 b_inc_x = e1.a / double_triangle_area;
  f32 b_inc_y = e1.b / double_triangle_area;
  f32 c =      eval2 / double_triangle_area;
  f32 c_inc_x = e2.a / double_triangle_area;
  f32 c_inc_y = e2.b / double_triangle_area;

  // Loop through the bounding box of pixels of the triangle
  for(u32 y_pixel = bottom_bb; y_pixel <= top_bb; y_pixel++)
  {
    // Save evaluation values for this row of pixels
    f32 start_eval0 = eval0;
    f32 start_eval1 = eval1;
    f32 start_eval2 = eval2;
    f32 start_a = a;
    f32 start_b = b;
    f32 start_c = c;

    for(u32 x_pixel = left_bb; x_pixel <= right_bb; x_pixel++)
    {
      u32 index = y_pixel * width + x_pixel;

      assert(x_pixel >= 0 && x_pixel < width);
      assert(y_pixel >= 0 && y_pixel < height);
      assert(index >= 0 && index < width * height);

      // Check if the point is inside the triangle by checking if edge equation evaluations are zero
      if((eval0 > 0 || (eval0 == 0 && e0.tl == true)) &&
         (eval1 > 0 || (eval1 == 0 && e1.tl == true)) &&
         (eval2 > 0 || (eval2 == 0 && e2.tl == true))
        )
      {
        Color color = Color(1.0f, 1.0f, 1.0f);
        
        // Calculate depth value for this pixel
        f32 depth = (a * p0.z) + (b * p1.z) + (c * p2.z);

        // Make sure this pixel has a lesser depth
        if(depth < depth_buffer[index])
        {
          f32 intensity0 = 1.0f;
          f32 intensity1 = 1.0f;
          f32 intensity2 = 1.0f;
          // Intensity is cos of the angle between the light source and the normal
          //intensity0 = dot(normals[0], (lightPos - centroid).unit());
          //intensity1 = dot(normals[1], (lightPos - centroid).unit());
          //intensity2 = dot(normals[2], (lightPos - centroid).unit());

          //f32 s = (a * textureCoords[0].x) + (b * textureCoords[1].x) + (c * textureCoords[2].x);
          //f32 t = (a * textureCoords[0].y) + (b * textureCoords[1].y) + (c * textureCoords[2].y);

          // Clamp the intensity to zero
          // The intensity may be negative if the light source is facing away from the normal
          if(intensity0 < 0.0f) intensity0 = 0.0f;
          if(intensity1 < 0.0f) intensity1 = 0.0f;
          if(intensity2 < 0.0f) intensity2 = 0.0f;

          // Interpolate the intensity of this pixel using barycentric coordinates
          f32 intensity = (a * intensity0) + (b * intensity1) + (c * intensity2);
          intensity = clamp(intensity, 0.0f, 1.0f);

#if 0
          color.r *= squared(intensity * depth);
          color.g *= squared(intensity * depth);
          color.b *= squared(intensity * depth);
#else
          color.r *= a;
          color.g *= b;
          color.b *= c;
#endif

          // Set the pixel depth in the depth buffer
          depth_buffer[index] = depth;

          // Set the final pixel color
          pixels[index] = color.pack();
        }
      }

      // Evaluate the next pixel's edge equation values incrementally
      eval0 += e0.a;
      eval1 += e1.a;
      eval2 += e2.a;

      // Barycentric coordinates incrementally
      a += a_inc_x;
      b += b_inc_x;
      c += c_inc_x;
    }

    // Reset the evaluations of the incremental values in the x direction
    eval0 = start_eval0;
    eval1 = start_eval1;
    eval2 = start_eval2;
    a = start_a;
    b = start_b;
    c = start_c;

    // Compute the y values incrementally
    eval0 += e0.b;
    eval1 += e1.b;
    eval2 += e2.b;
    a += a_inc_y;
    b += b_inc_y;
    c += c_inc_y;
  }
}

static void clip_polygon(u32 num_in_points, v4 *in_points, u32 *num_out_points, v4 *out_points, ClipPlane plane)
{
  v4 plane_equation;

  for(s32 i = 0; i < num_in_points; i++)
  {
    v4 first = in_points[i];
    v4 second = in_points[(i + 1) % num_in_points];

    switch(plane)
    {
      case LEFT_CLIP_PLANE:
      {
        plane_equation = v4(-1.0f, 0.0f, 0.0f, -1.0f);
        break;
      }

      case RIGHT_CLIP_PLANE:
      {
        plane_equation = v4(1.0f, 0.0f, 0.0f, -1.0f);
        break;
      }

      case BOTTOM_CLIP_PLANE:
      {
        plane_equation = v4(0.0f, -1.0f, 0.0f, -1.0f);
        break;
      }

      case TOP_CLIP_PLANE:
      {
        plane_equation = v4(0.0f, 1.0f, 0.0f, -1.0f);
        break;
      }

      case NEAR_CLIP_PLANE:
      {
        plane_equation = v4(0.0f, 0.0f, -1.0f, -1.0f);
        break;
      }

      case FAR_CLIP_PLANE:
      {
        plane_equation = v4(0.0f, 0.0f, 0.0f, -1.0f);
        break;
      }
    }

    f32 first_eval = dot(plane_equation, first);
    f32 second_eval = dot(plane_equation, second);
    bool first_outside = 0;
    bool second_outside = 0;

    if(first_eval > 0) first_outside = true;
    if(second_eval > 0) second_outside = true;

    // Add the first point if inside
    if(!first_outside)
    {
      out_points[*num_out_points] = first;
      (*num_out_points)++;
    }

    // Trivial rejection or acceptance
    if(!(first_outside ^ second_outside)) continue;


    f32 dist_first_to_plane = first_eval;
    f32 dist_first_to_second = first_eval - second_eval;
    f32 dist = dist_first_to_plane / dist_first_to_second;

    v4 clipped_point = first + dist * (second - first);
    clipped_point -= plane_equation * 0.001f * absf(dist_first_to_second);

    if(absf(clipped_point.x) > absf(clipped_point.w))
    {
      int b = 0;
    }

    out_points[*num_out_points] = clipped_point;
    (*num_out_points)++;
  }
}





void init_renderer()
{
  // GLFW and GLEW setup
  bool result;
  result = glfwInit();
  if(!result)
  {
    printf("Could not intialize glfw.\n");
    exit(1);
    return;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

  // TODO: Read in window width and height
  renderer_data.window = glfwCreateWindow(1920, 1080, "Software Renderer", NULL, NULL);
  if(!renderer_data.window)
  {
    printf("Could not make window\n");
    glfwTerminate();
    exit(1);
    return;
  }
  glfwMakeContextCurrent(renderer_data.window);

  glfwSetFramebufferSizeCallback(renderer_data.window, resize_callback);
  glfwSetKeyCallback(renderer_data.window, key_callback);
  glfwSetMouseButtonCallback(renderer_data.window, mousebutton_callback);

  GLenum glew_result = glewInit();
  if(glew_result != GLEW_OK)
  {
    printf("Could not initalize GLEW\n");
    exit(1);
    return;
  }

  // Use the screen width and height as viewport
  glfwGetFramebufferSize(renderer_data.window, (int *)(&renderer_data.screen_width), (int *)(&renderer_data.screen_height));
  resize_callback(renderer_data.window, renderer_data.screen_width, renderer_data.screen_height);



  // OpenGL setup
  s32 error = 0;
  // Create the PBO
  u32 byte_count = renderer_data.num_pixels * sizeof(u32);
  glCreateBuffers(1, &renderer_data.pboid);
  glNamedBufferStorage(renderer_data.pboid, byte_count, 0, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, renderer_data.pboid);

  // Create the texture object
  GLuint width = renderer_data.screen_width;
  GLuint height = renderer_data.screen_height;
  glCreateTextures(GL_TEXTURE_2D, 1, &renderer_data.texid);
  glBindTexture(GL_TEXTURE_2D, renderer_data.texid);
  glTextureStorage2D(renderer_data.texid, 1, GL_RGBA8, width, height);
  glTextureSubImage2D(renderer_data.texid, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // VAO
  glGenVertexArrays(1, &renderer_data.vaoid);
  glBindVertexArray(renderer_data.vaoid);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  f32 vertices[] =
  {
    // Shape            // Texture
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
  };
  // Send vertex data
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  // Vertex positions
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void *)0);
  glEnableVertexAttribArray(0);
  // Texture coordinates
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void *)(sizeof(f32) * 3));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);


  // Shaders
  const GLchar *vertex_shader_source =
  "#version 450 core\n                                " \
  "layout (location = 0) in vec2 vVertexPosition;     " \
  "layout (location = 1) in vec2 vVertexTexCoord;     " \
  "out vec2 vTexCoord;                                " \
  "void main()                                        " \
  "{                                                  " \
  "  gl_Position = vec4(vVertexPosition, 0.0, 1.0);   " \
  "  vTexCoord = vVertexTexCoord;                     " \
  "}                                                  ";

  const GLchar *frag_shader_source =
  "#version 450 core\n                      " \
  "in vec2 vTexCoord;                       " \
  "out vec4 fFragClr;                       " \
  "uniform sampler2D uTex0;                 " \
  "void main ()                             " \
  "{                                        " \
  "  vec4 clr = texture(uTex0, vTexCoord);  " \
  "  fFragClr = clr;                        " \
  "}                                        ";
  GLint success;
  s8 log[512];
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(vertex_shader, 512, NULL, log);
    printf("Could not compile vertex shader\n\n");
    printf(log);
  }
  GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
  glCompileShader(frag_shader);
  glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(frag_shader, 512, NULL, log);
    printf("Could not compile fragment shader\n\n");
    printf(log);
  }
  GLuint new_program = glCreateProgram();
  glAttachShader(new_program, vertex_shader);
  glAttachShader(new_program, frag_shader);
  glLinkProgram(new_program);
  GLint link_status;
  glGetProgramiv(new_program, GL_LINK_STATUS, &link_status);
  if(!success)
  {
    glGetShaderInfoLog(new_program, 512, NULL, log);
    printf("Could not link shaders\n\n");
    printf(log);
  }

  glUseProgram(new_program);




  // My setup
  renderer_data.clear_color = Color(0.0f, 0.0f, 0.0f, 1.0f).pack();
  clear_framebuffer();

  u32 size = renderer_data.num_pixels;
  renderer_data.depth_buffer = new f32[size];
  for(u32 i = 0; i < size; i++) renderer_data.depth_buffer[i] = 1.0f;

  renderer_data.pixel_info_buffer = new PixelInfo[size];

  renderer_data.model = new Model;
  renderer_data.model->position = v3(0.0f, 0.0f, 0.0);
  renderer_data.model->scale = v3(1.0f, 1.0f, 1.0f);
  renderer_data.model->rotation = 0.0;

  renderer_data.camera_position = v3(0.0f, 0.0f, 5.0f);
  renderer_data.camera_width = 60.0f;
  renderer_data.proj_type = true;
  renderer_data.near_plane = 1.0f;
  renderer_data.far_plane = 10.0f;

#if 0
  load_obj("meshes/head.obj", &renderer_data.model->vertices, 0, 0, &renderer_data.model->vertex_indices);
  normalize_mesh(&renderer_data.model->vertices);
#else
  v3 p0 = {-1.0f, -1.0f, 0.0f};
  v3 p1 = { 1.0f, -1.0f, -1.0f};
  v3 p2 = { 1.0f,  1.0f, -2.0f};
  //v3 p3 = {-1.0f,  1.0f, 0.0f};
  renderer_data.model->vertices.push_back(p0);
  renderer_data.model->vertices.push_back(p1);
  renderer_data.model->vertices.push_back(p2);
  //renderer_data.model->vertices.push_back(p3);
  renderer_data.model->vertex_indices.push_back(0);
  renderer_data.model->vertex_indices.push_back(1);
  renderer_data.model->vertex_indices.push_back(2);
  //renderer_data.model->vertex_indices.push_back(2);
  //renderer_data.model->vertex_indices.push_back(3);
  //renderer_data.model->vertex_indices.push_back(0);
#endif
}

// glDrawArrays
void render()
{
  u32 screen_width = renderer_data.screen_width;
  u32 screen_height = renderer_data.screen_height;

  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  clear_framebuffer();
  // Clear depth buffer
  for(u32 i = 0; i < renderer_data.num_pixels; i++) renderer_data.depth_buffer[i] = 1.0f;

  renderer_data.vertex_buffer.clear();
  renderer_data.index_buffer.clear();
  renderer_data.clipped_vertex_buffer.clear();
  renderer_data.clipped_index_buffer.clear();




  // Copy model vertices and indices to vertex and index buffer
  const Model *model = renderer_data.model;
  for(u32 i = 0; i < model->vertices.size(); i++)
  {
    renderer_data.vertex_buffer.push_back(v4(model->vertices[i], 1.0f));
  }
  for(u32 i = 0; i < model->vertex_indices.size(); i++)
  {
    renderer_data.index_buffer.push_back(model->vertex_indices[i]);
  }

  v3 position = renderer_data.model->position;
  v3 scale = renderer_data.model->scale;
  f32 rotation = renderer_data.model->rotation;

  mat4 pos_mat = translation_mat(position);
  mat4 scaling_mat = scale_mat(scale);
  mat4 rot_mat = z_axis_rotation_mat(rotation);
  mat4 world = pos_mat * rot_mat * scaling_mat;

  // u
  v3 right_axis = v3(1.0f, 0.0f, 0.0f);
  // v
  v3 up_axis = v3(0.0f, 1.0f, 0.0f);
  // w
  // This is looking away from the target
  v3 target_axis = v3(0.0f, 0.0f, 1.0f);
  mat4 view =
  {
    right_axis.x,  right_axis.y,  right_axis.z,  -dot(right_axis, renderer_data.camera_position),
    up_axis.x,     up_axis.y,     up_axis.z,     -dot(up_axis, renderer_data.camera_position),
    target_axis.x, target_axis.y, target_axis.z, -dot(target_axis, renderer_data.camera_position),
    0.0f, 0.0f, 0.0f, 1.0f,
  };
  mat4 ortho =
  {
    2.0f / renderer_data.camera_width, 0.0f, 0.0f, 0.0f,
    0.0f, (2.0f * renderer_data.aspect_ratio) / renderer_data.camera_width, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  // Distance from the near plane (must be positive)
  f32 n = renderer_data.near_plane;
  // Distance from the far plane (must be positive)
  f32 f = renderer_data.far_plane;
  float field_of_view = deg_to_rad(renderer_data.camera_width);
  f32 r = -(f + n) / (f - n);
  f32 s = -(2 * n * f) / (f - n);
  mat4 persp = 
  {
    (float)(1.0f / tan(field_of_view / 2.0f)) / renderer_data.aspect_ratio, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f / tan(field_of_view / 2.0f), 0.0f, 0.0f,
    0.0f, 0.0f, r, s,
    0.0f, 0.0f, -1.0f, 0.0f
  };

  mat4 projection;
  if(renderer_data.proj_type == false)
  {
    projection = ortho;
  }
  else
  {
    projection = persp;
  }
  // Vertex shader (model space to clip space)
  for(u32 i = 0; i < renderer_data.vertex_buffer.size(); i++)
  {
    v4 result = renderer_data.vertex_buffer[i];
    result = world * result;
    result = view * result;
    result = projection * result;
    renderer_data.vertex_buffer[i] = result;
  }
  
  // Clipping
#if 1
  // For each triangle
  for(u32 triangle_index = 0; triangle_index < renderer_data.index_buffer.size(); )
  {
    u32 p0_index = renderer_data.index_buffer[triangle_index++];
    u32 p1_index = renderer_data.index_buffer[triangle_index++];
    u32 p2_index = renderer_data.index_buffer[triangle_index++];

    // The three original triangle points
    v4 points[3] =
    {
      renderer_data.vertex_buffer[p0_index],
      renderer_data.vertex_buffer[p1_index],
      renderer_data.vertex_buffer[p2_index],
    };

#if 0
    u8 point_grid_positions[3];
    // Bit position x
    // 0 == inside
    // 1 == outside
    for(u32 i = 0; i < 3; i++)
    {
      u8 inside_test = 0;
      if(!(-points[i].x - points[i].w < 0)) inside_test |= 1 << 0; // Left
      if(!( points[i].x - points[i].w < 0)) inside_test |= 1 << 1; // Right
      if(!(-points[i].y - points[i].w < 0)) inside_test |= 1 << 2; // Bottom
      if(!( points[i].y - points[i].w < 0)) inside_test |= 1 << 3; // Top
      if(!(-points[i].z - points[i].w < 0)) inside_test |= 1 << 4; // Near
      if(!( points[i].z - points[i].w < 0)) inside_test |= 1 << 5; // Far

      point_grid_positions[i] = inside_test;
    }
#endif

    v4 a_points[6] = {};
    u32 num_a_points = 0;

    v4 b_points[6] = {};
    u32 num_b_points = 0;

    // Clip against each plane
    a_points[0] = points[0];
    a_points[1] = points[1];
    a_points[2] = points[2];
    num_a_points = 3;


    clip_polygon(num_a_points, a_points, &num_b_points, b_points, LEFT_CLIP_PLANE);

    num_a_points = 0;
    clip_polygon(num_b_points, b_points, &num_a_points, a_points, RIGHT_CLIP_PLANE);

    num_b_points = 0;
    clip_polygon(num_a_points, a_points, &num_b_points, b_points, BOTTOM_CLIP_PLANE);

    num_a_points = 0;
    clip_polygon(num_b_points, b_points, &num_a_points, a_points, TOP_CLIP_PLANE);

    num_b_points = 0;
    clip_polygon(num_a_points, a_points, &num_b_points, b_points, NEAR_CLIP_PLANE);

    num_a_points = 0;
    clip_polygon(num_b_points, b_points, &num_a_points, a_points, FAR_CLIP_PLANE);



    if(num_a_points)
    {
      for(s32 i = 0; i < num_a_points; i++)
      {
        renderer_data.clipped_vertex_buffer.push_back(a_points[i]);
      }
      u32 start_index = triangle_index - 3;
      for(s32 i = 1; i < num_a_points - 1; i++)
      {
        renderer_data.clipped_index_buffer.push_back(start_index);
        renderer_data.clipped_index_buffer.push_back(start_index + i);
        renderer_data.clipped_index_buffer.push_back(start_index + i + 1);
      }
    }
  }
#else
  for(u32 i = 0; i < renderer_data.vertex_buffer.size(); i++)
  {
    renderer_data.clipped_vertex_buffer.push_back(renderer_data.vertex_buffer[i]);
  }
  for(u32 i = 0; i < renderer_data.index_buffer.size(); i++)
  {
    renderer_data.clipped_index_buffer.push_back(renderer_data.index_buffer[i]);
  }

#endif


  // Perspective division (clip space to ndc space)
  for(u32 i = 0; i < renderer_data.clipped_vertex_buffer.size(); i++)
  {
    renderer_data.clipped_vertex_buffer[i] /= renderer_data.clipped_vertex_buffer[i].w; 
  }

  // Viewport transform (ndc space to viewport space)
  // Transform the vertex buffer
  for(u32 i = 0; i < renderer_data.clipped_vertex_buffer.size(); i++)
  {
    // Map the ndc to the screen coordinates
    v4 ndc = renderer_data.clipped_vertex_buffer[i];
#if 1
    if(ndc.x < -1.0f || ndc.x > 1.0f)
    {
      printf("ndc.x = %f, x should be between -1 and 1\n", ndc.x);
      assert(0);
    }
    if(ndc.y < -1.0f || ndc.y > 1.0f)
    {
      printf("ndc.y = %f, y should be between -1 and 1\n", ndc.y);
      assert(0);
    }
    if(ndc.z < -1.0f || ndc.z > 1.0f)
    {
      printf("ndc.z = %f, z should be between -1 and 1\n", ndc.z);
      assert(0);
    }
#else
    ndc.x = clamp(ndc.x, -1.0f, 1.0f);
    ndc.y = clamp(ndc.y, -1.0f, 1.0f);
    ndc.z = clamp(ndc.z, -1.0f, 1.0f);
#endif


    ndc += v4(1.0f, 1.0f, 0.0f, 0.0f);

    v4 screen_pos;
    screen_pos.x = ndc.x * (screen_width / 2.0f);
    screen_pos.y = ndc.y * (screen_height / 2.0f);
    screen_pos.z = (ndc.z + 1.0f) / 2.0f;
    screen_pos.w = ndc.w;


    renderer_data.clipped_vertex_buffer[i] = screen_pos;
  }





  // Rasterize triangles in buffers
  u32 *pixels = (u32 *)glMapNamedBuffer(renderer_data.pboid, GL_WRITE_ONLY);


  const std::vector<v4> &vertices = renderer_data.clipped_vertex_buffer;
  const std::vector<u32> &indices = renderer_data.clipped_index_buffer;
  for(u32 i = 0; i < indices.size(); )
  {
    v3 v[3];

    u32 index = indices[i++];
    v3 vertex = v3(vertices[index].x, vertices[index].y, vertices[index].z);
    v[0] = vertex;

    index = indices[i++];
    vertex = v3(vertices[index].x, vertices[index].y, vertices[index].z);
    v[1] = vertex;

    index = indices[i++];
    vertex = v3(vertices[index].x, vertices[index].y, vertices[index].z);
    v[2] = vertex;


    render_triangle(pixels, v[0], v[1], v[2], 0, 0, v3());
  }


  glUnmapNamedBuffer(renderer_data.pboid);

  // Send the texture data
  glTextureSubImage2D(renderer_data.texid, 0, 0, 0, screen_width, screen_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  glBindVertexArray(renderer_data.vaoid);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

// glClear
void clear_framebuffer()
{
  u32 *pixels = (u32 *)glMapNamedBuffer(renderer_data.pboid, GL_WRITE_ONLY);

  for(u32 i = 0; i < renderer_data.num_pixels; i++)
  {
    pixels[i] = renderer_data.clear_color;
  }

  glUnmapNamedBuffer(renderer_data.pboid);
}

void swap_buffers()
{
  glfwSwapBuffers(renderer_data.window);
}

void poll_events()
{
  glfwPollEvents();

  if(glfwWindowShouldClose(renderer_data.window)) exit(0);

  float speed = 0.025f;
  if(button_states['W'])
  {
    renderer_data.model->position.y += speed;
  }
  if(button_states['S'])
  {
    renderer_data.model->position.y -= speed;
  }
  if(button_states['A'])
  {
    renderer_data.model->position.x -= speed;
  }
  if(button_states['D'])
  {
    renderer_data.model->position.x += speed;
  }



#if 1
  if(button_states['I'])
  {
    renderer_data.model->scale.x -= speed;
    renderer_data.model->scale.y -= speed;
  }
  if(button_states['K'])
  {
    renderer_data.model->scale.x += speed;
    renderer_data.model->scale.y += speed;
  }
#else
  if(button_states['I'])
  {
    renderer_data.near_plane += 0.01f;
    printf("Near: %f\n", renderer_data.near_plane);
  }
  if(button_states['K'])
  {
    renderer_data.near_plane -= 0.01f;
    printf("Near: %f\n", renderer_data.near_plane);
  }
#endif

  if(button_states['J'])
  {
    renderer_data.model->rotation += speed;
  }
  if(button_states['L'])
  {
    renderer_data.model->rotation -= speed;
  }

  if(button_states['Z'])
  {
    renderer_data.camera_width -= 0.1f;
  }
  if(button_states['X'])
  {
    renderer_data.camera_width += 0.1f;
  }

  renderer_data.proj_type = true;
  if(button_states[' '])
  {
    renderer_data.proj_type = false;
  }

  double mouse_x, mouse_y;
  glfwGetCursorPos(renderer_data.window, &mouse_x, &mouse_y);
  mouse_y = -mouse_y + renderer_data.screen_height;

  static bool left_pressed = false;
  if(button_states[GLFW_MOUSE_BUTTON_LEFT] && !left_pressed)
  {
    printf("mouse_x, mouse_y: (%f, %f\n", mouse_x, mouse_y);
    print_pixel_info((u32)mouse_x, (u32)mouse_y);
  }
  left_pressed = button_states[GLFW_MOUSE_BUTTON_LEFT];
}

