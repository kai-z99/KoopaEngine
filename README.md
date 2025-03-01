# KoopaEngine

3D Programming has never been easier!

```c
#include "KoopaEngine.h"

int main()
{
	KoopaEngine ke = KoopaEngine();	

	while (!ke.shouldCloseWindow())
	{
		ke.BeginFrame();

		ke.DrawPlane(
			Vec3(0.0f, -4.0f, 0.0f),     //pos
			Vec3(5.0f, 0.0f, 3.0f),      //size
			Vec4(0.0f, 1.0f, 0.0f, 0.0f) //rotation
		);

		ke.DrawTriangle(
			Vec3(0.0f, 0.0f, 0.0f),
			Vec4(0.0f, 1.0f, 0.0f, 0.0f)
		);

		ke.DrawCube(
			Vec3(2.0f, 0.0f, 0.0f),
			Vec3(1.0f, 1.0f, 1.0f),
			Vec4(0.0f, 1.0f, 0.0f, 10.0f * ke.GetCurrentFrame())
		);
			
		ke.EndFrame();
	}

	return 0;
}
```

