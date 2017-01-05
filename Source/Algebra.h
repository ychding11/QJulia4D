

//***********************************************************************************
class mat3
//***********************************************************************************
{
  float m[3][3];

public:

  void identiy();

  // matrix multiplication
  mat3 operator * (  mat3& m2) const;

  // rotate 'angle' around axis (xa, ya, za)
  void rotateAroundAxis(float xa, float ya, float za, float angle);

  // access operator
  float& operator () ( int Row, int Col );

  // casting operators
  operator float* ();
};

