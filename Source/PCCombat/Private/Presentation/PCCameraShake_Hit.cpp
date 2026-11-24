#include "Presentation/PCCameraShake_Hit.h"

UPCCameraShake_Hit::UPCCameraShake_Hit()
{
	OscillationDuration = 0.22f;
	OscillationBlendInTime = 0.02f;
	OscillationBlendOutTime = 0.14f;

	RotOscillation.Pitch.Amplitude = 0.9f;
	RotOscillation.Pitch.Frequency = 38.f;
	RotOscillation.Yaw.Amplitude = 0.6f;
	RotOscillation.Yaw.Frequency = 31.f;

	LocOscillation.Z.Amplitude = 2.5f;
	LocOscillation.Z.Frequency = 34.f;
}
