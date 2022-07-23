#include "stdafx.h"
#include "Rectf.h"
#include "yasli/Archive.h"
using namespace yasli;

Rectf& Rectf::operator+=(const Rectf& rect) {
  if (rect.empty())
    ;
  else if (empty())
    *this = rect;
  else {
    min_.x = std::min(min_.x, rect.min_.x);
    min_.y = std::min(min_.y, rect.min_.y);
    max_.x = std::max(max_.x, rect.max_.x);
    max_.y = std::max(max_.y, rect.max_.y);
  }
  return *this;
}

Rectf operator*(const MatX2f& transform, const Rectf& rect) {
  Vect2f p[4] = {
    Vect2f(rect.left(), rect.top()),
    Vect2f(rect.right(), rect.top()),
    Vect2f(rect.right(), rect.bottom()),
    Vect2f(rect.left(), rect.bottom())
  };

  p[0] *= transform;
  p[1] *= transform;
  p[2] *= transform;
  p[3] *= transform;

  return Rectf(
    std::min(std::min(std::min(p[0].x, p[1].x), p[2].x), p[3].x),
    std::min(std::min(std::min(p[0].y, p[1].y), p[2].y), p[3].y),
    std::max(std::max(std::max(p[0].x, p[1].x), p[2].x), p[3].x),
    std::max(std::max(std::max(p[0].y, p[1].y), p[2].y), p[3].y));
}


void Rectf::serialize(Archive& ar)
{
  ar( min_.x, "", "^left" );
  ar( min_.y, "", "^top" );
  ar( max_.x, "", "^right" );
  ar( max_.y, "", "^bottom" );
}
