
#include "Algebra.h"
#include "math.h"


  //***********************************************************************************
void mat3::identiy()
  //***********************************************************************************
  {
    for (int j=0; j<3; j++)
    for (int i=0; i<3; i++)
      m[j][i] = 0;
    m[0][0] = 1;
    m[1][1] = 1;
    m[2][2] = 1;
  }
  
// matrix multiplication
  //***********************************************************************************
  mat3 mat3::operator * (  mat3& m2) const
  //***********************************************************************************
  {
    mat3 res;
    for (int j=0; j<3; j++)
    for (int i=0; i<3; i++)
    {
      res(j,i) = 0;
      for (int k=0; k<3; k++)
        res(j,i) += m[j][k] * m2(k,i);      
    }
    return res;
  }

    // access operator
    //***********************************************************************************
    float& mat3::operator () ( int Row, int Col )
    //***********************************************************************************
    {
      return m[Row][Col];
    }

    // casting operators
    //***********************************************************************************
    mat3::operator float* ()
    //***********************************************************************************
    {
      return &m[0][0];
    }

    // rotate 'angle' around axis (xa, ya, za)
    //***********************************************************************************
    void mat3::rotateAroundAxis(float xa, float ya, float za, float angle)
      //***********************************************************************************
    {
      // normalize axis for safety else rotation can include scale
      float t = sqrt(xa*xa+ya*ya+za*za);
      if (t!=0)
      {
        xa/=t;
        ya/=t;
        za/=t;
      }

      m[0][0] = 1+(1-cos(angle))*(xa*xa-1);
      m[1][1] = 1+(1-cos(angle))*(ya*ya-1);
      m[2][2] = 1+(1-cos(angle))*(za*za-1);

      m[0][1] = +za*sin(angle)+(1-cos(angle))*xa*ya;
      m[1][0] = -za*sin(angle)+(1-cos(angle))*xa*ya;

      m[0][2] = -ya*sin(angle)+(1-cos(angle))*xa*za;
      m[2][0] = +ya*sin(angle)+(1-cos(angle))*xa*za;

      m[1][2] = +xa*sin(angle)+(1-cos(angle))*ya*za;
      m[2][1] = -xa*sin(angle)+(1-cos(angle))*ya*za;
    }


