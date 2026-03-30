#ifndef __APPLICATION_H_
#define __APPLICATION_H_

class Application
{
private:

public:

	void OnResize();
	void Update(float a_fDeltaTime, float a_fTotalTime);
	void Draw(float a_fDeltaTime, float a_fTotalTime);
};

#endif // __APPLICATION_H_

