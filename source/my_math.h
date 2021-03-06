#pragma once

#include "types.h"

#include <math.h> // sqrt, cos, sin, atan2f

#define PI 3.14159265f
#define squared(a) (a * a)
#define deg_to_rad(a) (a * (PI / 180.0f));

///////////////////////////////////////////////////////////////////////////////
// vector structs
///////////////////////////////////////////////////////////////////////////////
struct v2
{
  f32 x;
  f32 y;

  // Default constructor
  v2() : x(0.0f), y(0.0f) { }

  // Copy constructor
  v2(const v2 &v) : x(v.x), y(v.y) { }

  // Non default constructor
  v2(f32 in_x, f32 in_y) : x(in_x), y(in_y) { }
};

struct v3
{
  f32 x;
  f32 y;
  f32 z;

  // Default constructor
  v3() : x(0.0f), y(0.0f), z(0.0f) { }

  // Copy constructor
  v3(const v3 &v) : x(v.x), y(v.y), z(v.z) { }

  // Non default constructor
  v3(f32 in_x, f32 in_y, f32 in_z) : x(in_x), y(in_y), z(in_z) { }

  v3(v2 v, f32 a) : x(v.x), y(v.y), z(a) { }
  v3(f32 a, v2 v) : x(a),   y(v.x), z(v.y) { }
};

struct v4
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;

  // Default constructor
  v4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }

  // Copy constructor
  v4(const v4 &v) : x(v.x), y(v.y), z(v.z), w(v.w) { }

  // Non default constructor
  v4(f32 in_x, f32 in_y, f32 in_z, f32 in_w) : x(in_x), y(in_y), z(in_z), w(in_w) { }

  v4(v2 v, f32 a, f32 b) : x(v.x), y(v.y), z(a),   w(b)   { }
  v4(f32 a, v2 v, f32 b) : x(a),   y(v.x), z(v.y), w(b)   { }
  v4(f32 a, f32 b, v2 v) : x(a),   y(b),   z(v.x), w(v.y) { }
  v4(v3 v, f32 a) : x(v.x), y(v.y), z(v.z), w(a)   { }
  v4(f32 a, v3 v) : x(a),   y(v.x), z(v.y), w(v.z) { }
};

///////////////////////////////////////////////////////////////////////////////
// vector operations
///////////////////////////////////////////////////////////////////////////////

static v2 operator+(v2 a, v2 b) { return v2(a.x + b.x, a.y + b.y); }
static v3 operator+(v3 a, v3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static v4 operator+(v4 a, v4 b) { return v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }

static v2 operator-(v2 a, v2 b) { return v2(a.x - b.x, a.y - b.y); }
static v3 operator-(v3 a, v3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static v4 operator-(v4 a, v4 b) { return v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }

// Unary negation
static v2 operator-(v2 a) { return v2(-a.x, -a.y); }
static v3 operator-(v3 a) { return v3(-a.x, -a.y, -a.z); }
static v4 operator-(v4 a) { return v4(-a.x, -a.y, -a.z, -a.w); }

// Dot product
static f32 operator*(v2 a, v2 b) { return (a.x * b.x) + (a.y * b.y); }
static f32 operator*(v3 a, v3 b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }
static f32 operator*(v4 a, v4 b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w); }
static f32 dot(v2 a, v2 b) { return a * b; }
static f32 dot(v3 a, v3 b) { return a * b; }
static f32 dot(v4 a, v4 b) { return a * b; }

static v2 operator*(v2 a, f32 scalar) { return v2(a.x * scalar, a.y * scalar); }
static v3 operator*(v3 a, f32 scalar) { return v3(a.x * scalar, a.y * scalar, a.z * scalar); }
static v4 operator*(v4 a, f32 scalar) { return v4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar); }
static v2 operator*(f32 scalar, v2 a) { return v2(a.x * scalar, a.y * scalar); }
static v3 operator*(f32 scalar, v3 a) { return v3(a.x * scalar, a.y * scalar, a.z * scalar); }
static v4 operator*(f32 scalar, v4 a) { return v4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar); }

static v2 operator/(v2 a, f32 scalar) { return v2(a.x / scalar, a.y / scalar); }
static v3 operator/(v3 a, f32 scalar) { return v3(a.x / scalar, a.y / scalar, a.z / scalar); }
static v4 operator/(v4 a, f32 scalar) { return v4(a.x / scalar, a.y / scalar, a.z / scalar, a.w / scalar); }

static v2 &operator+=(v2 &a, v2 b) { a.x += b.x; a.y += b.y; return a; }
static v3 &operator+=(v3 &a, v3 b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
static v4 &operator+=(v4 &a, v4 b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }

static v2 &operator-=(v2 &a, v2 b) { a.x -= b.x; a.y -= b.y; return a; }
static v3 &operator-=(v3 &a, v3 b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }
static v4 &operator-=(v4 &a, v4 b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w; return a; }

static v2 &operator*=(v2 &a, f32 b) { a.x -= b; a.y -= b; return a; }
static v3 &operator*=(v3 &a, f32 b) { a.x -= b; a.y -= b; a.z -= b; return a; }
static v4 &operator*=(v4 &a, f32 b) { a.x -= b; a.y -= b; a.z -= b; a.w -= b; return a; }

static v2 &operator/=(v2 &a, f32 b) { a.x /= b; a.y /= b; return a; }
static v3 &operator/=(v3 &a, f32 b) { a.x /= b; a.y /= b; a.z /= b; return a; }
static v4 &operator/=(v4 &a, f32 b) { a.x /= b; a.y /= b; a.z /= b; a.w /= b; return a; }

// Gets the length of the vector
static f32 length(v2 v)
{
  return (f32)sqrt(squared(v.x) + squared(v.y));
}
static f32 length(v3 v)
{
  return (f32)sqrt(squared(v.x) + squared(v.y) + squared(v.z));
}
static f32 length(v4 v)
{
  return (f32)sqrt(squared(v.x) + squared(v.y) + squared(v.z) + squared(v.w));
}

// Gets the squared length of this vector
static f32 length_squared(v2 v)
{
  return squared(v.x) + squared(v.y);
}
static f32 length_squared(v3 v)
{
  return squared(v.x) + squared(v.y) + squared(v.z);
}
static f32 length_squared(v4 v)
{
  return squared(v.x) + squared(v.y) + squared(v.z) + squared(v.w);
}

// Returns a unit vector from this vector.
static v2 unit(v2 v)
{
  return v / length(v);
}
static v3 unit(v3 v)
{
  return v / length(v);
}
static v4 unit(v4 v)
{
  return v / length(v);
}

// Returns this vector clamped by max length
static v2 clamp_length(v2 v, f32 max_length)
{
  f32 len = length(v);
  if(len > max_length)
  {
    v = v / len;
    v = v * max_length;
  }

  return v;
}
static v3 clamp_length(v3 v, f32 max_length)
{
  f32 len = length(v);
  if(len > max_length)
  {
    v = v / len;
    v = v * max_length;
  }

  return v;
}
static v4 clamp_length(v4 v, f32 max_length)
{
  f32 len = length(v);
  if(len > max_length)
  {
    v = v / len;
    v = v * max_length;
  }

  return v;
}


///////////////////////////////////////////////////////////////////////////////
// v2 specific operations
///////////////////////////////////////////////////////////////////////////////

// Returns a vector that is perpendicular to this vector. This specific
// normal will be rotated 90 degrees clockwise.
static v2 find_normal(v2 a)
{
  return v2(a.y, -a.x);
}

// Returns this vector rotated by the angle in radians
static v2 rotated(v2 a, f32 angle)
{
  v2 v;

  v.x = a.x * (f32)cos(angle) - a.y * (f32)sin(angle);
  v.y = a.x * (f32)sin(angle) + a.y * (f32)cos(angle);

  return v;
}

// Returns the angle this vector is pointing at in radians. If the vector is
// pointing straight right, the angle is 0. If left, the angle is PI / 2.0f
// etc.
//      PI/2
//       |
// PI <-- --> 0
//       |
//     -PI/2
static f32 angle(v2 a)
{
  return atan2f(a.y, a.x);
}


///////////////////////////////////////////////////////////////////////////////
// v3 specific operations
///////////////////////////////////////////////////////////////////////////////

static v3 cross(v3 a, v3 b)
{
  v3 v;
  v.x = (a.y * b.z) - (a.z * b.y);
  v.y = (a.z * b.x) - (a.x * b.z);
  v.z = (a.x * b.y) - (a.y * b.x);
  return v;
}







///////////////////////////////////////////////////////////////////////////////
// matrix structs
///////////////////////////////////////////////////////////////////////////////

struct mat4
{
  f32 v[4][4];

#if 0
  mat4()
  {
    v[0][0] = 1.0f; v[0][1] = 0.0f; v[0][2] = 0.0f; v[0][3] = 0.0f;
    v[1][0] = 0.0f; v[1][1] = 1.0f; v[1][2] = 0.0f; v[1][3] = 0.0f;
    v[2][0] = 0.0f; v[2][1] = 0.0f; v[2][2] = 1.0f; v[2][3] = 0.0f;
    v[3][0] = 0.0f; v[3][1] = 0.0f; v[3][2] = 0.0f; v[3][3] = 1.0f;
  }
#endif

  const f32 *operator[](u32 i) const
  {
    return v[i];
  }
  f32 *operator[](u32 i)
  {
    return v[i];
  }
};

///////////////////////////////////////////////////////////////////////////////
// matrix operations
///////////////////////////////////////////////////////////////////////////////

// This funciton was made only for the matrix-vector multiplication
static f32 dot4v(const f32 *a, v4 b)
{
  return (a[0] * b.x) + (a[1] * b.y) + (a[2] * b.z) + (a[3] * b.w);
}
static v4 operator*(const mat4 &lhs, v4 rhs)
{
  v4 result;

  result.x = dot4v(lhs[0], rhs);
  result.y = dot4v(lhs[1], rhs);
  result.z = dot4v(lhs[2], rhs);
  result.w = dot4v(lhs[3], rhs);

  return result;
}

static mat4 operator*(const mat4 &lhs, const mat4 &rhs)
{
  mat4 product;

  // Loop through each spot in the resulting matrix
  for(u32 row = 0; row < 4; row++)
  {
    for(u32 col = 0; col < 4; col++)
    {
      // Dot the row and column for the given slot
      f32 dot = 0.0f;
      for(u32 i = 0; i < 4; i++)
      {
        dot += lhs.v[row][i] * rhs.v[i][col];
      }

      product[row][col] = dot;
    }
  }

  return product;
}

static mat4 translation_mat(v3 offset)
{
  mat4 result = 
  {
    1.0f, 0.0f, 0.0f, offset.x,
    0.0f, 1.0f, 0.0f, offset.y,
    0.0f, 0.0f, 1.0f, offset.z,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  return result;
}

static mat4 scale_mat(v3 scale)
{
  mat4 result = 
  {
    scale.x, 0.0f, 0.0f, 0.0f,
    0.0f, scale.y, 0.0f, 0.0f,
    0.0f, 0.0f, scale.z, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  return result;
}

static mat4 x_axis_rotation_mat(f32 radians)
{
  mat4 result = 
  {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, (f32)cos(radians), (f32)-sin(radians), 0.0f,
    0.0f, (f32)sin(radians), (f32)cos(radians), 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  return result;
}

static mat4 y_axis_rotation_mat(f32 radians)
{
  mat4 result = 
  {
    (f32)cos(radians), 0.0f, (f32)sin(radians), 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    (f32)-sin(radians), 0.0f, (f32)cos(radians), 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  return result;
}

static mat4 z_axis_rotation_mat(f32 radians)
{
  mat4 result = 
  {
    (f32)cos(radians), (f32)-sin(radians), 0.0f, 0.0f,
    (f32)sin(radians), (f32)cos(radians), 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  return result;
}


///////////////////////////////////////////////////////////////////////////////
// common operations
///////////////////////////////////////////////////////////////////////////////

static s32 min(s32 a, s32 b)
{
  return (a < b) ? a : b; 
}
static s32 min(s32 a, s32 b, s32 c)
{
  return min(a, min(b, c)); 
}
static f32 min(f32 a, f32 b)
{
  return (a < b) ? a : b; 
}
static f32 min(f32 a, f32 b, f32 c)
{
  return min(a, min(b, c)); 
}

static s32 max(s32 a, s32 b)
{
  return (a > b) ? a : b; 
}
static s32 max(s32 a, s32 b, s32 c)
{
  return max(a, max(b, c)); 
}
static f32 max(f32 a, f32 b)
{
  return (a > b) ? a : b; 
}
static f32 max(f32 a, f32 b, f32 c)
{
  return max(a, max(b, c)); 
}

static s32 clamp(s32 a, s32 min, s32 max)
{
  if(a < min) return min;
  if(a > max) return max;
  return a;
}
static f32 clamp(f32 a, f32 min, f32 max)
{
  if(a < min) return min;
  if(a > max) return max;
  return a;
}

static f32 absf(f32 a)
{
  return (a < 0.0f) ? -a : a;
}

