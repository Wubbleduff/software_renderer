//------------------------------------------------------------------------------
//
// File Name:  Vector2D.h
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// Public Structures:
//------------------------------------------------------------------------------

#define PI 3.14159265f

class v2
{
public:
  float x;
  float y;

  // Default constructor
  v2();

  // Copy constructor
  v2(const v2 &v);

  // Non default constructor
  v2(float inX, float inY);

  // Gets the length of the vector
  float Length() const;

  // Gets the squared length of the vector
  float LengthSquared() const;

  // Returns a unit vector from this vector. If the vector is the zero
  // vector, returns the given vector.
  v2 Unit() const;

  // Returns this vector clamped by max length
  v2 ClampLength(float maxLength) const;

  // Returns a vector that is perpendicular to this vector. This specific
  // normal will be rotated 90 degrees clockwise.
  v2 FindNormal() const;

  // Returns this vector rotated by the angle in radians
  v2 Rotated(float angle) const;

  // Returns the angle this vector is pointing at in radians. If the vector is
  // pointing straight right, the angle is 0. If left, the angle is PI / 2.0f
  // etc.
  //      PI/2
  //       |
  // PI <-- --> 0
  //       |
  //     -PI/2
  float Angle() const;

  // Adds two vectors
  v2 operator+(v2 rhs) const;

  // Subtracts two vectors (v1 - v2)
  v2 operator-(v2 rhs) const;

  // Unary negation
  v2 operator-() const;

  // Adds a vector to this one
  v2 &operator+=(v2 rhs);

  // Subtracts a vector from this one
  v2 &operator-=(v2 rhs);

  // Scales a vector by the number
  v2 &operator*=(float rhs);

  // Scales a vector by 1 / number
  v2 &operator/=(float rhs);

  // Calculates the dot product between this and the other vector
  float operator*(v2 v) const;

  // Returns this vector scaled by the scalar
  v2 operator*(float scalar) const;

  // Returns this vector scaled by the 1 / scalar
  v2 operator/(float scalar) const;

private:
};

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

// Returns this vector scaled by the scalar
v2 operator*(float scalar, const v2 rhs);

// Calculates the dot product between this and the other vector
float dot(v2 a, v2 b);
