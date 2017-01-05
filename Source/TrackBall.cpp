#include "math.h"
#include "Algebra.h"
#include "TrackBall.h"


#ifndef min
  #define min(x, y) ((x < y)? x : y)
#endif
#ifndef max
  #define max(x, y) ((x > y)? x : y)
#endif



//***********************************************************************************
TrackBall::TrackBall()
//***********************************************************************************
  {
    orientation.identiy();
    rotate.identiy();
  }


//***********************************************************************************
void TrackBall::PointOnTrackBall(float &x, float &y, float &z, int mouseX, int mouseY, int mouseWidth, int mouseHeight)
//***********************************************************************************
{
    x =  (float)(mouseX - mouseWidth/2)  / (float)min(mouseWidth, mouseHeight)*2;
    y =  (float)(mouseY - mouseHeight/2) / (float)min(mouseWidth, mouseHeight)*2;

    float rr = x*x+y*y;

    if ( rr < 1)
      z = sqrt(1- rr);
    else
    {
      x = x / sqrt(rr);
      y = y / sqrt(rr);
      z = 0;
    }
}


//***********************************************************************************
  void TrackBall::DragStart(int mouseX, int mouseY, int mouseWidth, int mouseHeight)
//***********************************************************************************
  {
    PointOnTrackBall(x0, y0, z0, mouseX, mouseY, mouseWidth, mouseHeight);
  }
//***********************************************************************************
  void TrackBall::DragMove(int mouseX, int mouseY, int mouseWidth, int mouseHeight)
//***********************************************************************************
  {
    PointOnTrackBall(x1, y1, z1, mouseX, mouseY, mouseWidth, mouseHeight);

    // rotation axis  = cross(p0, p1)
    float xa = y0*z1 - y1*z0;
    float ya = z0*x1 - z1*x0;
    float za = x0*y1 - x1*y0;

    // rotation angle = acos(dot(p0,p1))
    float angle = 2*acos(min(max(x0*x1+y0*y1+z0*z1,-1),1)); // multiply angle by 2 for 360 degree range

    // rotate matrix around axis
    rotate.rotateAroundAxis(xa, ya, za, angle); 
  }

  //***********************************************************************************
  void TrackBall::DragEnd()
   //***********************************************************************************
  {  
    // multiply current orientation with dragged rotation
    orientation = orientation*rotate;
    rotate.identiy();
  }

  //***********************************************************************************
  mat3 TrackBall::GetRotationMatrix()
  //***********************************************************************************
  {
    return orientation*rotate;
  }

