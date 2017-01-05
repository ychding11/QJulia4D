


//***********************************************************************************
class TrackBall
//***********************************************************************************
{
    float x0, y0, z0;  // point on trackball when mouse down at start rotation mouse drag
    float x1, y1, z1;  // point on trackball at mouse drag positioin

    mat3 rotate;       // delta rotation while dragging
    mat3 orientation;  // orientation of trackball

public:


  TrackBall();

  void PointOnTrackBall(float &x, float &y, float &z, int mouseX, int mouseY, int mouseWidth, int mouseHeight);

  void DragStart(int mouseX, int mouseY, int mouseWidth, int mouseHeight);
  void  DragMove(int mouseX, int mouseY, int mouseWidth, int mouseHeight);
  void DragEnd();

  mat3 GetRotationMatrix();
};