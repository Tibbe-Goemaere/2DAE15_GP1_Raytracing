//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"
#include "iostream"

using namespace dae;
using namespace LightUtils;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();
	float fovAngle{ tanf(TO_RADIANS * camera.fovAngle / 2.f) };
	float aspectRatio{static_cast<float>(m_Width/m_Height)};
	const Matrix cameraToWorld{ camera.CalculateCameraToWorld()};

	for (int px{}; px < m_Width; ++px)
	{
		const float cx{ static_cast<float>(((2 * (px + 0.5f) - m_Width) / m_Height) * fovAngle * aspectRatio) };
		for (int py{}; py < m_Height; ++py)
		{
			const float cy{ static_cast<float>(((m_Height - 2 * (py + 0.5f)) / m_Height) * fovAngle) };
			Vector3 rayDirection{Vector3::UnitX * cx + Vector3::UnitY * cy + Vector3::UnitZ};
			rayDirection = cameraToWorld.TransformVector(rayDirection);
			rayDirection.Normalize();

			Ray viewRay{ camera.origin,rayDirection };

			ColorRGB finalColor{};
			HitRecord closestHit{};
			
			pScene->GetClosestHit(viewRay, closestHit);
			finalColor = materials[closestHit.materialIndex]->Shade();
			finalColor.MaxToOne();
			if (closestHit.didHit)
			{
				float offSet{ 0.0001f };
				Ray hitToLight{};
				hitToLight.origin = closestHit.origin + closestHit.normal * (offSet * 2.f);

				for (const Light& light : lights)
				{
					hitToLight.direction = GetDirectionToLight(light, hitToLight.origin).Normalized();
					hitToLight.min = offSet;
					hitToLight.max = GetDirectionToLight(light, hitToLight.origin).Magnitude();

					if (pScene->DoesHit(hitToLight))
					{
						
						finalColor *= 0.5f;
					}
				}
			
			}

			
			
			
			//Update Color in Buffer
			//finalColor = materials[closestHit.materialIndex]->Shade();
			//finalColor.MaxToOne();

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
	
	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::SwitchModes()
{
	const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
	if (pKeyboardState[SDL_SCANCODE_F2])
	{
		m_ShadowsEnabled != m_ShadowsEnabled;
	}

	if (pKeyboardState[SDL_SCANCODE_F3])
	{
		switch (m_CurrentLightingMode)
		{
		case dae::Renderer::LightingMode::ObservedArea:
			m_CurrentLightingMode = LightingMode::Radiance;
			break;
		case dae::Renderer::LightingMode::Radiance:
			m_CurrentLightingMode = LightingMode::BRDF;
			break;
		case dae::Renderer::LightingMode::BRDF:
			m_CurrentLightingMode = LightingMode::Combined;
			break;
		case dae::Renderer::LightingMode::Combined:
			m_CurrentLightingMode = LightingMode::ObservedArea;
			break;
		default:
			break;
		}
	}
}
