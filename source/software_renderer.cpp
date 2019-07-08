#include "software_renderer.h"
#include "my_math.h"
#include "asset_loading.h"
#include "input.h"
#include "profiling.h"

#include "logging.h"

#include <assert.h> // assert
#include <vector>

struct Color
{
  f32 r, g, b, a;

  Color()
  {
    r = 0.0f;
    g = 0.0f;
    b = 0.0f;
    a = 0.0f;
  }

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

    //u32 result = (red << 0) | (green << 8) | (blue << 16) | (alpha << 24);
    u32 result = (blue << 0) | (green << 8) | (red << 16) | (alpha << 24);
    return result;
  }
};

struct Model
{
  v3 position;
  v3 scale;
  f32 rotation;


  std::vector<v3> vertices;
  std::vector<u32> vertex_indices;
  std::vector<v3> normals;
};

struct PixelInfo
{
  u32 x;
  u32 y;
  Color final_color;
  v3 triangle_vertices[3];
};

// This struct is for the vertex buffer. It will contain the vertex along with any attributes of that vertex (normal, material, UV, etc)
struct Vertex
{
  v4 vertex;
  v3 normal;
};

enum RenderMode
{
  RENDER_MODE_TRIANGLES,
  RENDER_MODE_LINES
};

struct RendererData
{
  u32 *frame_buffer;
  u32 screen_width;
  u32 screen_height;
  u32 num_pixels;
  f32 aspect_ratio;

  RenderMode mode;

  v3 camera_position;
  f32 camera_width;
  bool proj_type; // false is ortho, true is perspective
  f32 near_plane;
  f32 far_plane;

  f32 *depth_buffer;
  PixelInfo *pixel_info_buffer;

  u32 clear_color;

  Model *model;


  std::vector<Vertex> vertex_buffer;
  std::vector<u32> index_buffer;

  std::vector<Vertex> clipped_vertex_buffer;
  std::vector<u32> clipped_index_buffer;
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

  log_file("mouse x: %d", x);
  log_file("mouse y: %d", y);
  log_file("R: %f, G: %f, B: %f", pixel->final_color.r, pixel->final_color.g, pixel->final_color.b);
  log_file("V0: (%f, %f, %f)", pixel->triangle_vertices[0].x, pixel->triangle_vertices[0].y, pixel->triangle_vertices[0].z);
  log_file("V1: (%f, %f, %f)", pixel->triangle_vertices[1].x, pixel->triangle_vertices[1].y, pixel->triangle_vertices[1].z);
  log_file("V2: (%f, %f, %f)", pixel->triangle_vertices[2].x, pixel->triangle_vertices[2].y, pixel->triangle_vertices[2].z);
  log_file("model pos: %f, %f", renderer_data.model->position.x, renderer_data.model->position.y);
  log_file("model scale: %f, %f", renderer_data.model->scale.x, renderer_data.model->scale.y);
  log_file("model rot: %f", renderer_data.model->rotation);

  log_file("\n\n");

}

static void update_stuff()
{
  f32 speed = 0.055f;
  if(key_state('W'))
  {
    renderer_data.model->position.y += speed;
  }
  if(key_state('S'))
  {
    renderer_data.model->position.y -= speed;
  }
  if(key_state('A'))
  {
    renderer_data.model->position.x -= speed;
  }
  if(key_state('D'))
  {
    renderer_data.model->position.x += speed;
  }



#if 1
  if(key_state('I'))
  {
    renderer_data.model->scale.x -= speed;
    renderer_data.model->scale.y -= speed;
  }
  if(key_state('K'))
  {
    renderer_data.model->scale.x += speed;
    renderer_data.model->scale.y += speed;
  }
#else
  if(key_state('I'))
  {
    renderer_data.near_plane += 0.01f;
    printf("Near: %f\n", renderer_data.near_plane);
  }
  if(key_state('K'))
  {
    renderer_data.near_plane -= 0.01f;
    printf("Near: %f\n", renderer_data.near_plane);
  }
#endif

  if(key_state('J'))
  {
    renderer_data.model->rotation += speed;
  }
  if(key_state('L'))
  {
    renderer_data.model->rotation -= speed;
  }

  if(key_state('Z'))
  {
    renderer_data.camera_width -= 0.1f;
  }
  if(key_state('X'))
  {
    renderer_data.camera_width += 0.1f;
  }

#if 0
  renderer_data.proj_type = true;
  if(key_state(' '))
  {
    renderer_data.proj_type = false;
  }
#endif

  static bool left_click = false;
  v2 pos = mouse_window_position();
  pos.y -= renderer_data.screen_height;
  pos.y *= -1.0f;
  if(mouse_state(0) && !left_click)
  {
    log_file("Mouse position: %f, %f", pos.x, pos.y);
    print_pixel_info((u32)pos.x, (u32)pos.y);
  }
  left_click = mouse_state(0);

  if(key_state('M'))
  {
    renderer_data.mode = RENDER_MODE_LINES;
  }
  else
  {
    renderer_data.mode = RENDER_MODE_TRIANGLES;
  }
}

// Computes the per vertex normals for a given mesh
static void compute_vertex_normals(const std::vector<v3> *in_vertices, const std::vector<u32> *in_indices, std::vector<v3> *out_normals)
{
  const std::vector<v3> &vertices = *in_vertices;
  const std::vector<unsigned> &indices = *in_indices;

  if(vertices.size() == 0) return;

  // Allocate buffers
  std::vector<v3> &normals = *out_normals;
  normals.reserve(vertices.size());
  normals.resize(vertices.size());

  std::vector<unsigned> sums;
  sums.reserve(vertices.size());
  sums.resize(vertices.size());

  // Initalize to zero
  for(unsigned i = 0; i < normals.size(); i++) normals[i] = v3(0.0f, 0.0f, 0.0f);
  for(unsigned i = 0; i < sums.size(); i++) sums[i] = 0;

  // Find the sum of all normals per vertex
  for(unsigned i = 0; i < indices.size(); )
  {
    unsigned p0_index = indices[i++];
    unsigned p1_index = indices[i++];
    unsigned p2_index = indices[i++];

    v3 p0 = vertices[p0_index];
    v3 p1 = vertices[p1_index];
    v3 p2 = vertices[p2_index];

    v3 normal = cross(p1 - p0, p2 - p0);
    normal = unit(normal);

    normals[p0_index] += normal;
    normals[p1_index] += normal;
    normals[p2_index] += normal;

    sums[p0_index]++;
    sums[p1_index]++;
    sums[p2_index]++;
  }

  // Divide to find average normal per vertex
  for(unsigned i = 0; i < normals.size(); i++)
  {
    normals[i] /= (f32)sums[i];
  }
}

void render_line_bresenham(u32 x1, u32 y1, u32 x2, u32 y2, Color color)
{
  assert(x1 >= 0);
  assert(x1 < renderer_data.screen_width);
  assert(x2 >= 0);
  assert(x2 < renderer_data.screen_width);

  assert(y1 >= 0);
  assert(y1 < renderer_data.screen_height);
  assert(y2 >= 0);
  assert(y2 < renderer_data.screen_height);

  s32 dx = x2 - x1; 
  s32 dy = y2 - y1;

  s32 absdx = (dx < 0) ? -dx : dx;
  s32 absdy = (dy < 0) ? -dy : dy;

  // Deal with a vertical line
  if(dx == 0)
  {
    s32 y_step = (y1 < y2) ? 1 : -1;
    while(y1 != y2)
    {
      renderer_data.frame_buffer[y1 * renderer_data.screen_width + x1] = color.pack();
      y1 += y_step;
    }

    return;
  }

  renderer_data.frame_buffer[y1 * renderer_data.screen_width + x1] = color.pack();

  s32 counter = 0;



  // Check if slope is less than or greater than 1
  if(absdy <= absdx)
  {
    // Slope <= 1

    u32 start_x;
    u32 start_y;
    s32 y_step;
    if(x1 < x2)
    {
      start_x = x1;
      start_y = y1;
      y_step = (y1 < y2) ? 1 : -1;
    }
    else
    {
      start_x = x2;
      start_y = y2;
      y_step = (y1 < y2) ? -1 : 1;
    }

    u32 current_x = start_x;
    u32 current_y = start_y;


    while(current_x < start_x + absdx)
    {
      current_x++;
      counter += absdy;

      if(counter > absdx)
      {
        counter -= absdx;
        current_y += y_step;
      }

      renderer_data.frame_buffer[current_y * renderer_data.screen_width + current_x] = color.pack();
    }
  }
  else
  {
    // Slope > 1

    u32 start_x;
    u32 start_y;
    s32 x_step;
    if(y1 < y2)
    {
      start_x = x1;
      start_y = y1;
      x_step = (x1 < x2) ? 1 : -1;
    }
    else
    {
      start_x = x2;
      start_y = y2;
      x_step = (x1 < x2) ? -1 : 1;
    }

    u32 current_x = start_x;
    u32 current_y = start_y;


    while(current_y < start_y + absdy)
    {
      current_y++;
      counter += absdx;

      if(counter > absdy)
      {
        counter -= absdy;
        current_x += x_step;
      }

      renderer_data.frame_buffer[current_y * renderer_data.screen_width + current_x] = color.pack();
    }
  }
}

// Render a triangle in viewport pixel space between points p0, p1, p2 with the given color (depending on the render mode)
// p0, p1, p2 will only show if in counter-clockwise order
// If there are normals and/or texture coords, there must be 3 normals and/or 3 texture coordinates given
static void render_triangle(u32 *pixels, v3 p0, v3 p1, v3 p2, v3 *normals, v2 *texture_coords)
#if 1
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

  assert(left_bb >= 0);
  assert(bottom_bb >= 0);
  assert(right_bb < width);
  assert(top_bb < height);

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

  // Loop through the bounding box of pixels of the triangle
  for(u32 y_pixel = bottom_bb; y_pixel <= top_bb; y_pixel++)
  {
    for(u32 x_pixel = left_bb; x_pixel <= right_bb; x_pixel++)
    {
      u32 index = y_pixel * width + x_pixel;

      f32 eval0 = e0.a * x_pixel + e0.b * y_pixel + e0.c;
      f32 eval1 = e1.a * x_pixel + e1.b * y_pixel + e1.c;
      f32 eval2 = e2.a * x_pixel + e2.b * y_pixel + e2.c;

      // Check if the point is inside the triangle by checking if edge equation evaluations are zero
      if((eval0 > 0.0f || (eval0 == 0.0f && e0.tl == true)) &&
         (eval1 > 0.0f || (eval1 == 0.0f && e1.tl == true)) &&
         (eval2 > 0.0f || (eval2 == 0.0f && e2.tl == true))
        )
      {
        Color color = Color(1.0f, 1.0f, 1.0f);

        f32 double_triangle_area = eval0 + eval1 + eval2;
        f32 a = eval0 / double_triangle_area;
        f32 b = eval1 / double_triangle_area;
        f32 c = eval2 / double_triangle_area;
        
        // Calculate depth value for this pixel
        f32 depth = (a * p0.z) + (b * p1.z) + (c * p2.z);

        // Make sure this pixel has a lesser depth
        if(depth < depth_buffer[index])
        {
          v3 light_pos = v3(0.0f, 0.0f, 1.0f);
          f32 intensity0 = dot(normals[0], unit(light_pos));
          f32 intensity1 = dot(normals[1], unit(light_pos));
          f32 intensity2 = dot(normals[2], unit(light_pos));

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

#if 1
          color.r *= squared(intensity) * 0.8f;
          color.g *= squared(intensity) * 0.0f;
          color.b *= squared(intensity) * 1.0f;
#else
          color.r *= a;
          color.g *= b;
          color.b *= c;
#endif

          // Set the pixel depth in the depth buffer
          depth_buffer[index] = depth;

          // Set the final pixel color
          pixels[index] = color.pack();


          renderer_data.pixel_info_buffer[index].x = x_pixel;
          renderer_data.pixel_info_buffer[index].y = y_pixel;
          renderer_data.pixel_info_buffer[index].final_color = color;
          renderer_data.pixel_info_buffer[index].triangle_vertices[0] = p0;
          renderer_data.pixel_info_buffer[index].triangle_vertices[1] = p1;
          renderer_data.pixel_info_buffer[index].triangle_vertices[2] = p2;
        }
      }
    }
  }
}
#else
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

  assert(left_bb >= 0);
  assert(bottom_bb >= 0);
  assert(right_bb < width);
  assert(top_bb < height);

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
      if((eval0 > 0.0f || (eval0 == 0.0f && e0.tl == true)) &&
         (eval1 > 0.0f || (eval1 == 0.0f && e1.tl == true)) &&
         (eval2 > 0.0f || (eval2 == 0.0f && e2.tl == true))
        )
      {
        Color color = Color(1.0f, 1.0f, 1.0f);
        
        // Calculate depth value for this pixel
        f32 depth = (a * p0.z) + (b * p1.z) + (c * p2.z);

        // Make sure this pixel has a lesser depth
        if(depth < depth_buffer[index])
        {
#if 0
          f32 intensity0 = 1.0f;
          f32 intensity1 = 1.0f;
          f32 intensity2 = 1.0f;
#else
          // Intensity is cos of the angle between the light source and the normal
          v3 light_pos = v3(0.0f, 0.0f, 1.0f);
          f32 intensity0 = dot(normals[0], unit(light_pos));
          f32 intensity1 = dot(normals[1], unit(light_pos));
          f32 intensity2 = dot(normals[2], unit(light_pos));
#endif

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

#if 1
          color.r *= squared(intensity) * 0.8f;
          color.g *= squared(intensity) * 0.0f;
          color.b *= squared(intensity) * 1.0f;
#else
          color.r *= a;
          color.g *= b;
          color.b *= c;
#endif

          // Set the pixel depth in the depth buffer
          depth_buffer[index] = depth;

          // Set the final pixel color
          pixels[index] = color.pack();


          renderer_data.pixel_info_buffer[index].x = x_pixel;
          renderer_data.pixel_info_buffer[index].y = y_pixel;
          renderer_data.pixel_info_buffer[index].final_color = color;
          renderer_data.pixel_info_buffer[index].triangle_vertices[0] = p0;
          renderer_data.pixel_info_buffer[index].triangle_vertices[1] = p1;
          renderer_data.pixel_info_buffer[index].triangle_vertices[2] = p2;
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
#endif

static void clip_polygon(ClipPlane plane, u32 num_in_points, Vertex *in_points, u32 *num_out_points, Vertex *out_points)
{
  v4 plane_equation;

  for(u32 i = 0; i < num_in_points; i++)
  {
    Vertex first = in_points[i];
    Vertex second = in_points[(i + 1) % num_in_points];

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
      { //                                |
        // ??? does this need to be zero? V
        plane_equation = v4(0.0f, 0.0f, 1.0f, -1.0f);
        break;
      }
    }

    f32 first_eval = dot(plane_equation, first.vertex);
    f32 second_eval = dot(plane_equation, second.vertex);
    bool first_outside = 0;
    bool second_outside = 0;

    if(first_eval > 0)  first_outside = true;
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

    const float EPSILON = 0.001f;
    if(first_eval > second_eval) dist += EPSILON;
    else                         dist -= EPSILON;
    
    
    // Interpolate vertex
    v4 clipped_point = first.vertex + dist * (second.vertex - first.vertex);
    v3 clipped_normal = first.normal + dist * (second.normal - first.normal);

    Vertex clipped_vertex;
    clipped_vertex.vertex = clipped_point;
    clipped_vertex.normal = clipped_normal;

    out_points[*num_out_points] = clipped_vertex;
    (*num_out_points)++;
  }
}





void init_renderer(u32 *frame_buffer, u32 width, u32 height)
{
  renderer_data.frame_buffer = frame_buffer;
  renderer_data.screen_width = width;
  renderer_data.screen_height = height;
  renderer_data.num_pixels = width * height;
  renderer_data.aspect_ratio = (f32)width / (f32)height;



  renderer_data.clear_color = Color(0.0f, 0.0f, 0.0f, 0.0f).pack();
  clear_frame_buffer();

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

#if 1
  load_obj("meshes/head.obj", &renderer_data.model->vertices, 0, 0, &renderer_data.model->vertex_indices);
  normalize_mesh(&renderer_data.model->vertices);
#else
  v3 p0 = {-1.0f, -1.0f, 0.0f};
  v3 p1 = { 1.0f, -1.0f, 0.0f};
  v3 p2 = { 1.0f,  1.0f, 0.0f};
  v3 p3 = {-1.0f,  1.0f, 0.0f};
  //v3 p0 = {-1000.0f, -1000.0f, 0.0f};
  //v3 p1 = { 1000.0f, -1000.0f, 0.0f};
  //v3 p2 = { 1000.0f,  1000.0f, 0.0f};
  //v3 p3 = {-1000.0f,  1000.0f, 0.0f};
  renderer_data.model->vertices.push_back(p0);
  renderer_data.model->vertices.push_back(p1);
  renderer_data.model->vertices.push_back(p2);
  renderer_data.model->vertices.push_back(p3);
  renderer_data.model->vertex_indices.push_back(0);
  renderer_data.model->vertex_indices.push_back(1);
  renderer_data.model->vertex_indices.push_back(2);
  renderer_data.model->vertex_indices.push_back(2);
  renderer_data.model->vertex_indices.push_back(3);
  renderer_data.model->vertex_indices.push_back(0);
#endif
  compute_vertex_normals(&renderer_data.model->vertices, &renderer_data.model->vertex_indices, &renderer_data.model->normals);

  renderer_data.model->position = v3();
  renderer_data.model->scale = v3(6, 6, 1.0f);
  renderer_data.model->rotation = deg_to_rad(90);
}

// glDrawArrays
void render()
{
  u32 screen_width = renderer_data.screen_width;
  u32 screen_height = renderer_data.screen_height;

  clear_frame_buffer();
  // Clear depth buffer
  for(u32 i = 0; i < renderer_data.num_pixels; i++) renderer_data.depth_buffer[i] = 1.0f;

  // Clear pixel info buffer
  for(u32 i = 0; i < renderer_data.num_pixels; i++) renderer_data.pixel_info_buffer[i] = PixelInfo();

  renderer_data.vertex_buffer.clear();
  renderer_data.index_buffer.clear();
  renderer_data.clipped_vertex_buffer.clear();
  renderer_data.clipped_index_buffer.clear();




  // Copy model vertices and indices to vertex and index buffer
  const Model *model = renderer_data.model;
  for(u32 i = 0; i < model->vertices.size(); i++)
  {
    Vertex v;
    v.vertex = v4(model->vertices[i], 1.0f);
    v.normal = model->normals[i];
    renderer_data.vertex_buffer.push_back(v);
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
  f32 field_of_view = deg_to_rad(renderer_data.camera_width);
  f32 r = -(f + n) / (f - n);
  f32 s = -(2 * n * f) / (f - n);
  mat4 persp = 
  {
    (f32)(1.0f / tan(field_of_view / 2.0f)) / renderer_data.aspect_ratio, 0.0f, 0.0f, 0.0f,
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
  time_block("1: vertex transformation");
  for(u32 i = 0; i < renderer_data.vertex_buffer.size(); i++)
  {
    v4 result = renderer_data.vertex_buffer[i].vertex;
    result = world * result;
    result = view * result;
    result = projection * result;
    renderer_data.vertex_buffer[i].vertex = result;
  }
  end_time_block();
  
  // Clipping
#if 1
  time_block("2: clipping");
  // For each triangle
  for(u32 triangle_index = 0; triangle_index < renderer_data.index_buffer.size(); )
  {
    // The three original triangle point indices
    u32 point_indices[3];
    point_indices[0] = renderer_data.index_buffer[triangle_index++];
    point_indices[1] = renderer_data.index_buffer[triangle_index++];
    point_indices[2] = renderer_data.index_buffer[triangle_index++];

    // The three original triangle points
    Vertex points[3];
    points[0] = renderer_data.vertex_buffer[point_indices[0]];
    points[1] = renderer_data.vertex_buffer[point_indices[1]];
    points[2] = renderer_data.vertex_buffer[point_indices[2]];

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

    // Make two buffers for added clipped points
    // One buffer defines the polygon, the other stores the clipped result
    Vertex a_points[6] = {};
    u32 num_a_points = 0;

    Vertex b_points[6] = {};
    u32 num_b_points = 0;

    // Clip against each plane
    a_points[0] = points[0];
    a_points[1] = points[1];
    a_points[2] = points[2];
    num_a_points = 3;


    clip_polygon(LEFT_CLIP_PLANE, num_a_points, a_points, &num_b_points, b_points);

    num_a_points = 0;
    clip_polygon(RIGHT_CLIP_PLANE, num_b_points, b_points, &num_a_points, a_points);

    num_b_points = 0;
    clip_polygon(BOTTOM_CLIP_PLANE, num_a_points, a_points, &num_b_points, b_points);

    num_a_points = 0;
    clip_polygon(TOP_CLIP_PLANE, num_b_points, b_points, &num_a_points, a_points);

    num_b_points = 0;
    clip_polygon(NEAR_CLIP_PLANE, num_a_points, a_points, &num_b_points, b_points);

    num_a_points = 0;
    clip_polygon(FAR_CLIP_PLANE, num_b_points, b_points, &num_a_points, a_points);



    // TODO: This will add redundant vertices and not optimize using indices
    //       For each index, there will be a vertex with it
    //
    //       Reuse vertices that are already in the clipped buffer
    if(num_a_points)
    {
      for(u32 i = 0; i < num_a_points; i++)
      {
        renderer_data.clipped_vertex_buffer.push_back(a_points[i]);
      }
      u32 start_index = renderer_data.clipped_vertex_buffer.size() - num_a_points;
      for(u32 i = 1; i < num_a_points - 1; i++)
      {
        renderer_data.clipped_index_buffer.push_back(start_index);
        renderer_data.clipped_index_buffer.push_back(start_index + i);
        renderer_data.clipped_index_buffer.push_back(start_index + i + 1);
      }
    }
  }
  end_time_block();
#else // Clipping
  for(u32 i = 0; i < renderer_data.vertex_buffer.size(); i++)
  {
    renderer_data.clipped_vertex_buffer.push_back(renderer_data.vertex_buffer[i]);
  }
  for(u32 i = 0; i < renderer_data.index_buffer.size(); i++)
  {
    renderer_data.clipped_index_buffer.push_back(renderer_data.index_buffer[i]);
  }

#endif // Clipping


  // Perspective division (clip space to ndc space)
  time_block("3: perspective division");
  for(u32 i = 0; i < renderer_data.clipped_vertex_buffer.size(); i++)
  {
    renderer_data.clipped_vertex_buffer[i].vertex /= renderer_data.clipped_vertex_buffer[i].vertex.w; 
  }
  end_time_block();

  // Viewport transform (ndc space to viewport space)
  // Transform the vertex buffer
  time_block("4: viewport transform");
  for(u32 i = 0; i < renderer_data.clipped_vertex_buffer.size(); i++)
  {
    // Map the ndc to the screen coordinates
    v4 ndc = renderer_data.clipped_vertex_buffer[i].vertex;

    if(ndc.x < -1.0f || ndc.x > 1.0f)
    {
      log_file("ndc.x = %f, x should be between -1 and 1\n", ndc.x);
      assert(0);
    }
    if(ndc.y < -1.0f || ndc.y > 1.0f)
    {
      log_file("ndc.y = %f, y should be between -1 and 1\n", ndc.y);
      assert(0);
    }
    if(ndc.z < -1.0f || ndc.z > 1.0f)
    {
      log_file("ndc.z = %f, z should be between -1 and 1\n", ndc.z);
      assert(0);
    }


    ndc += v4(1.0f, 1.0f, 0.0f, 0.0f);

    v4 screen_pos;
    screen_pos.x = ndc.x * (screen_width / 2.0f);
    screen_pos.y = ndc.y * (screen_height / 2.0f);
    screen_pos.z = (ndc.z + 1.0f) / 2.0f;
    screen_pos.w = ndc.w;

#if 0
    if(screen_pos.x < 0) screen_pos.x += 0.5f;
    if(screen_pos.x >= screen_width) screen_pos.x -= 0.5f;
    if(screen_pos.y < 0) screen_pos.y += 0.5f;
    if(screen_pos.y >= screen_height) screen_pos.y -= 0.5f;
#endif


    renderer_data.clipped_vertex_buffer[i].vertex = screen_pos;
  }
  end_time_block();





  // Rasterize triangles in buffers
  u32 *pixels = renderer_data.frame_buffer;


  const std::vector<Vertex> &vertices = renderer_data.clipped_vertex_buffer;
  const std::vector<u32> &indices = renderer_data.clipped_index_buffer;

  time_block("5: draw all triangles");
  for(u32 i = 0; i < indices.size(); )
  {
    v3 v[3];
    v3 n[3];

    u32 index = indices[i++];
    v3 vertex = v3(vertices[index].vertex.x, vertices[index].vertex.y, vertices[index].vertex.z);
    v[0] = vertex;
    n[0] = vertices[index].normal;

    index = indices[i++];
    vertex = v3(vertices[index].vertex.x, vertices[index].vertex.y, vertices[index].vertex.z);
    v[1] = vertex;
    n[1] = vertices[index].normal;

    index = indices[i++];
    vertex = v3(vertices[index].vertex.x, vertices[index].vertex.y, vertices[index].vertex.z);
    v[2] = vertex;
    n[2] = vertices[index].normal;




    if(renderer_data.mode == RENDER_MODE_TRIANGLES)
    {
      time_block("6: rasterize triangle");
      render_triangle(pixels, v[0], v[1], v[2], n, 0);
      end_time_block();
    }
    else
    {
      render_line_bresenham((u32)v[0].x, (u32)v[0].y, (u32)v[1].x, (u32)v[1].y, Color(0.0f, 0.0f, 1.0f));
      render_line_bresenham((u32)v[1].x, (u32)v[1].y, (u32)v[2].x, (u32)v[2].y, Color(0.0f, 0.0f, 1.0f));
      render_line_bresenham((u32)v[2].x, (u32)v[2].y, (u32)v[0].x, (u32)v[0].y, Color(0.0f, 0.0f, 1.0f));
    }

  }
  end_time_block();

  update_stuff();
}

// glClear
void clear_frame_buffer()
{
  u32 *pixels = renderer_data.frame_buffer;

  for(u32 i = 0; i < renderer_data.num_pixels; i++)
  {
    pixels[i] = renderer_data.clear_color;
  }
}

void swap_buffers()
{
}


