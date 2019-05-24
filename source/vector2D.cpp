//------------------------------------------------------------------------------
//
// File Name:  Vector2D.cpp
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "Vector2D.h"

#include <math.h>

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

// Default constructor
v2::v2() : x(0.0f), y(0.0f)
{
}

// Copy constructor
v2::v2(const v2 &v)
{
  x = v.x;
  y = v.y;
}

// Non default constructor
v2::v2(float inX, float inY) : x(inX), y(inY)
{
}

// Gets the length of the vector
float v2::Length() const
{
  // Using the pythagorean theorem to find the length
  return (float)sqrt(x * x + y * y);
}

// Gets the squared length of this vector
float v2::LengthSquared() const
{
  return x * x + y * y;
}

// Returns a unit vector from this vector. If the vector is the zero
// vector, returns the given vector.
v2 v2::Unit() const
{
  v2 v(*this); // A copy of the given vector

  // Make sure the length of the given vector is not zero to avoid dividing
  // by 0
  if(x == 0.0f && y == 0.0f)
  {
    return *this;
  }

  // Divide the vector's componenets by the vector's length. This will
  // give a vector that always has length 1.
  v.x /= this->Length();
  v.y /= this->Length();

  return v;
}

// Returns this vector clamped by max length
v2 v2::ClampLength(float maxLength) const
{
  v2 v(*this);

  if(this->Length() > maxLength)
  {
    // Make a unit vector
    v = v.Unit();

    // Scale the vector by the max length
    v = v * maxLength;
  }

  return v;
}

// Returns a vector that is perpendicular to this vector. This specific
// normal will be rotated 90 degrees clockwise.
v2 v2::FindNormal() const
{
  return v2(y, -x);
}

// Returns this vector rotated by the angle in radians
v2 v2::Rotated(float angle) const
{
  v2 v; // A temporary vector to hold the results

  v.x = x * (float)cos(angle) - y * (float)sin(angle);
  v.y = x * (float)sin(angle) + y * (float)cos(angle);

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
float v2::Angle() const
{
  return atan2f(y, x);
}

// Adds two vectors
v2 v2::operator+(v2 rhs) const
{
  return v2(x + rhs.x, y + rhs.y);
}

// Subtracts two vectors (v1 - v2)
v2 v2::operator-(v2 rhs) const
{
  return v2(x - rhs.x, y - rhs.y);
}

// Unary negation
v2 v2::operator-() const
{
  return v2(-x, -y);
}

// Adds a vector to this one
v2 &v2::operator+=(v2 rhs)
{
  x += rhs.x;
  y += rhs.y;
  
  return *this;
}

// Subtracts a vector from this one
v2 &v2::operator-=(v2 rhs)
{
  x -= rhs.x;
  y -= rhs.y;

  return *this;
}

// Scales a vector by the number
v2 &v2::operator*=(float rhs)
{
  x *= rhs;
  y *= rhs;

  return *this;
}

// Scales a vector by 1 / number
v2 &v2::operator/=(float rhs)
{
  x /= rhs;
  y /= rhs;

  return *this;
}

// Calculates the dot product between two vectors
float v2::operator*(v2 v) const
{
  return (x * v.x) + (y * v.y);
}

// Returns this vector scaled by the scalar
v2 v2::operator*(float scalar) const
{
  return v2(x * scalar, y * scalar);
}

// Returns this vector scaled by the scalar
v2 operator*(float scalar, v2 rhs)
{
  return v2(rhs.x * scalar, rhs.y * scalar);
}

// Returns this vector scaled by the 1 / scalar
v2 v2::operator/(float scalar) const
{
  return v2(x / scalar, y / scalar);
}

// Calculates the dot product between this and the other vector
float dot(v2 a, v2 b)
{
  return a * b;
}

