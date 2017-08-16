

#ifndef GAME_CLIENT_COMPONENTS_CAMERA_H
#define GAME_CLIENT_COMPONENTS_CAMERA_H
#include <base/vmath.h>
#include <game/client/component.h>

class CCamera : public CComponent
{
	enum
	{
		CAMTYPE_UNDEFINED=-1,
		CAMTYPE_SPEC,
		CAMTYPE_PLAYER,
	};

	int m_CamType;
	vec2 m_PrevCenter;

public:
	vec2 m_Center;
	float m_Zoom;

	CCamera();
	virtual void OnRender();
};

#endif
