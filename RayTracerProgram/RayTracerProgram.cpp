// RayTracerProgram.cpp : Defines the entry point for the application.
//

#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#ifndef GLM_FORCE_SSE42
#define GLM_FORCE_SSE42 1
#endif // !GLM_FORCE_SSE42

#define _USE_MATH_DEFINES // for C++
#include <cmath>
#include <Windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include "RayTracerProgram.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vector>

class Image
{
public:
	static const size_t numberOfBytesPerPixel = 4;

	Image(size_t width, size_t height)
		:mWidth(width),
		mHeight(height)
	{
		mData = reinterpret_cast<UINT*>(malloc(width * height * numberOfBytesPerPixel));
	}

	~Image()
	{
		free(mData);
	}

	UINT32* data() { return mData; }
	size_t numberOfBytesPerRow() { return mWidth * numberOfBytesPerPixel; }
	size_t numberOfBytes() { return numberOfBytesPerRow() * mHeight; }

	UINT32* operator[](int i) {
		return mData + i * mWidth;
	}
private:
	UINT32* mData;
	size_t mWidth;
	size_t mHeight;
};

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

union Pixel
{
	struct {
		UINT8 r,g,b,a;
	} rgba;
	UINT32 data;
};

struct Ray
{
	glm::vec3 o;
	glm::vec3 d;
};

struct HitResult
{
	glm::vec3 p;
	glm::vec3 n;
	float t;
};

class Material
{
public:
	Material(glm::vec3 kd, glm::vec3 ks, float km)
		: mKd(kd),
		mKs(ks),
		mKm(km)
	{
	}
	~Material();

	glm::vec3& kd() { return mKd; }
	glm::vec3& ks() { return mKs; }
	float km() { return mKm; }

private:
	glm::vec3 mKd;
	glm::vec3 mKs;
	float mKm;
};

Material::~Material()
{
}

class Geometry
{
public:
	Geometry();
	virtual ~Geometry() = 0;

	virtual bool hit(Ray& const ray, float t1, float t2, HitResult& hitResult) = 0;

	virtual Material& material() { return *mat; }
	virtual void attachMaterial(Material* material) { mat = material; }
private:
	Material* mat{ nullptr };
};

Geometry::Geometry()
{
}

Geometry::~Geometry()
{
	if (mat)
	{
		delete mat;
	}
}

class Sphere: public Geometry
{
public:
	Sphere (glm::vec3 c, float r)
		: Geometry(),
		mC(c),
		mR(r)
	{

	};

	virtual bool hit(Ray& const ray, float t1, float t2, HitResult& hitResult) override
	{
		float dDotEMinusC = glm::dot(ray.d, ray.o - mC);
		float dDotD = glm::dot(ray.d, ray.d);
		glm::vec3 eMinusC = ray.o - mC;
		float discriminant = dDotEMinusC * dDotEMinusC - dDotD * (glm::dot(eMinusC, eMinusC) - mR * mR);
		if (discriminant < 0)
		{
			return false;
		}
		
		float t = (-dDotEMinusC - sqrtf(discriminant)) / dDotD;
		if (t < t1 || t > t2)
		{
			return false;
		}

		hitResult.p = ray.o + t * ray.d;
		hitResult.n = (hitResult.p - mC) / mR;
		hitResult.t = t;

		return true;
	}


private:
	glm::vec3 mC;
	float mR;
};

class Plain: public Geometry
{
public:
	Plain(glm::vec3 n, glm::vec3 a)
		: mN(n),
		mA(a)
	{
	};
	~Plain();

	virtual bool hit(Ray& const ray, float t1, float t2, HitResult& hitResult) override;
	

private:
	glm::vec3 mN;
	glm::vec3 mA;
};

Plain::~Plain()
{
}

bool Plain::hit(Ray& const ray, float t1, float t2, HitResult& hitResult)
{
	float dDotN = glm::dot(ray.d, mN);
	
	if (fabs(dDotN) < FLT_EPSILON)
	{
		return false;
	}

	float t = (glm::dot(mA, mN) - glm::dot(ray.o, mN)) / dDotN;
	if (t < t1 || t > t2)
	{
		return false;
	}

	hitResult.p = ray.o + t * ray.d;
	hitResult.n = mN;
	hitResult.t = t;

	return true;
}


class Scene
{
public:
	Scene();
	~Scene();
	glm::vec3 rayColor(Ray& ray, float t1, float t2, int n);
	void addPointLight(glm::vec3 pointLight)
	{
		mPointLightList.push_back(pointLight);
	}

	void addGeometry(Geometry* geometry)
	{
		mGeometryList.push_back(geometry);
	}
private:
	std::vector<glm::vec3> mPointLightList;
	std::vector<Geometry*> mGeometryList;
};

Scene::Scene()
{
}

Scene::~Scene()
{
}

glm::vec3 Scene::rayColor(Ray& ray, float t1, float t2, int n = 0)
{
	if (n == 2)
	{
		return glm::vec3(0);
	}

	HitResult hitResult;
	HitResult finalHitResult;
	bool hit = false;
	Geometry* hitGeometry = nullptr;
	for (Geometry* geometry : mGeometryList)
	{
		if (geometry->hit(ray, t1, t2, hitResult))
		{
			hit = true;
			t2 = hitResult.t;
			finalHitResult = hitResult;
			hitGeometry = geometry;
		}
	}

	if (hit)
	{
		glm::vec3 color = glm::vec3(0.f);

		Material& material = hitGeometry->material();
		color = 0.4f * material.kd();
		for (glm::vec3 lightPoint : mPointLightList)
		{
			glm::vec3 shadowRayD = lightPoint - finalHitResult.p;
			Ray shadowRay = {
				finalHitResult.p,
				shadowRayD
			};
			bool inShadow = false;
			for (Geometry* geometry : mGeometryList)
			{
				if (geometry->hit(shadowRay, 1e-4, INFINITY, hitResult))
				{
					inShadow = true;
					break;
				}
			}
			if (inShadow)
			{
				break;
			}
			glm::vec3 lightVec = glm::normalize(shadowRayD);
			glm::vec3 viewVec = glm::normalize(ray.o - finalHitResult.p);
			glm::vec3 halfVec = glm::normalize(lightVec + viewVec);
			
			color +=
				material.kd() * glm::max(0.f, glm::dot(finalHitResult.n, lightVec)) +
				material.ks() * (float)pow(glm::dot(finalHitResult.n, halfVec), 100)
				;

		}

		glm::vec3 d = glm::normalize(ray.d);
		glm::vec3 reflectionRayD = d - (2.f * glm::dot(d, finalHitResult.n)) * finalHitResult.n;
		Ray reflectionRay = {
			finalHitResult.p,
			reflectionRayD
		};
		glm::vec3 reflectColor = material.km() * rayColor(reflectionRay, 1e-2, INFINITY, n + 1);
		color += reflectColor;
		color = glm::clamp(color, glm::vec3(0.f), glm::vec3(255.f));

		return color;
	}

	if (n == 0)
	{
		return glm::vec3(20.f);
	}
	else {
		return glm::vec3(0.f);
	}
}

class Window
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		Window* pThis = NULL;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* params = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = reinterpret_cast<Window*>(params->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		else
		{
			pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (pThis)
		{
			return pThis->handleMessage(hwnd, uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	BOOL createWindow(
		LPCWSTR lpTitle,
		DWORD dwStyle,
		int width = CW_USEDEFAULT,
		int height = CW_USEDEFAULT,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		DWORD dwExStyle = 0,
		HWND parent = 0,
		HMENU menu = 0
	)
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = Window::WindowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = L"main window";
		RegisterClass(&wc);

		mHwnd = CreateWindowEx(
			dwExStyle,
			wc.lpszClassName,
			lpTitle,
			dwStyle,
			x, y, width, height,
			parent,
			menu,
			wc.hInstance,
			this);

		return mHwnd ? TRUE : FALSE;
	}

	void show()
	{
		ShowWindow(mHwnd, SW_SHOW);
	}

private:
	HRESULT createGraphicsResources()
	{
		HRESULT hr = S_OK;
		if (pRenderTarget == NULL)
		{
			RECT frame;
			GetClientRect(mHwnd, &frame);
			D2D1_SIZE_U size = D2D1::SizeU(frame.right, frame.bottom);

			hr = pFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(mHwnd, size),
				&pRenderTarget);
		}

		return hr;
	}

	void discardGraphicsResources()
	{
		SafeRelease(&pRenderTarget);
	}

	void drawImage()
	{
		glm::vec4 vec = glm::vec4(1.0);
		RECT frame;
		GetClientRect(mHwnd, &frame);
		size_t width = frame.right;
		size_t height = frame.bottom;
		mImage = new Image(width, height);
		union Pixel pixel;
		pixel.rgba.a = 255;

		//glm::mat4 matrix = glm::mat4(1.0f);
		//float angle = -M_PI / 9.f;
		//matrix = glm::translate(matrix, glm::vec3(0, 0, -600.f));
		//matrix = glm::rotate(matrix, angle, glm::vec3(1.f, 0, 0));
		//glm::vec4 rayOrigin = matrix * glm::vec4(0, 0, 600, 1);
		//glm::vec4 rayOrigin = glm::vec4(0.f, 100.f, 0.f, 1.f);
		Ray ray;
		ray.o = glm::vec3(0.f, 100.f, 0.f);
		glm::vec3 baseU = glm::vec3(1.f, 0, 0);
		glm::vec3 baseW = glm::normalize(ray.o - glm::vec3(0, 0, -600.f));
		glm::vec3 baseV = glm::normalize(glm::cross(baseW, baseU));

		float d = 400.f;

		float l = -100.f;
		float b = -100.f;
		float pixelSpacingX = 200.f / width;
		float pixelSpacingY = 200.f / height;
		float u, v;

		Scene scene;
		glm::vec3 lightPoint = glm::vec3(-600.f, 1000.f, 0.f);
		scene.addPointLight(lightPoint);
		glm::vec3 lightPoint1 = glm::vec3(600.f, 1000.f, 0.f);
		scene.addPointLight(lightPoint1);
		Sphere sphere = Sphere(glm::vec3(0.f, 0, -500.f), 20.f);
		Material* mat = new Material(glm::vec3(255.f, 0.f, 0.f), glm::vec3(180.f), 0.f);
		sphere.attachMaterial(mat);
		scene.addGeometry(&sphere);

		Sphere sphere1 = Sphere(glm::vec3(40.f, 0, -530.f), 20.f);
		Material* mat1 = new Material(glm::vec3(0.f, 0.f, 190.f), glm::vec3(180.f), 0.f);
		sphere1.attachMaterial(mat1);
		scene.addGeometry(&sphere1);
		Plain plain = Plain(glm::vec3(0, 1.f, 0), glm::vec3(0, -20.f, 0.f));
		Material* mat2 = new Material(glm::vec3(80.f), glm::vec3(0.f), 0.3f);
		plain.attachMaterial(mat2);
		scene.addGeometry(&plain);

		for (size_t row = 0; row < height; row++)
		{
			v = b + (row + 0.5) * pixelSpacingY;
			for (size_t col = 0; col < width; col++)
			{
				u = l + (col + 0.5) * pixelSpacingX;
				ray.d = -d * baseW + u * baseU + v * baseV;
				if (row == 17 && col == 0)
				{
					printf("shit happened");
				}
				glm::vec3 color = scene.rayColor(ray, 0, INFINITY);
				pixel.rgba.r = color.r;
				pixel.rgba.g = color.g;
				pixel.rgba.b = color.b;
				(*mImage)[row][col] = pixel.data;
			}
		}
	}

	HWND mHwnd{ 0 };
	ID2D1Factory* pFactory { nullptr };
	ID2D1HwndRenderTarget* pRenderTarget { nullptr };
	Image* mImage{ nullptr };
	LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg)
		{
			case WM_CREATE:
			{
				if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
				{
					return -1;
				}
				return 0;
			}

			case WM_DESTROY:
			{
				PostQuitMessage(0);
				return 0;
			}

			case WM_PAINT:
			{
				HRESULT hr = createGraphicsResources();
				if (SUCCEEDED(hr))
				{
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hwnd, &ps);
					pRenderTarget->BeginDraw();
					if (mImage == nullptr)
					{
						drawImage();
					}

					D2D1_SIZE_U size = pRenderTarget->GetPixelSize();
					FLOAT dpix = 0;
					FLOAT dpiy = 0;
					pRenderTarget->GetDpi(&dpix, &dpiy);
					ID2D1Bitmap* bitmap;
					D2D1_PIXEL_FORMAT  pixelFormat = {
						DXGI_FORMAT_R8G8B8A8_UNORM,
						D2D1_ALPHA_MODE_PREMULTIPLIED
					};
					D2D1_BITMAP_PROPERTIES bitmapProps = {
						pixelFormat,
						dpix,
						dpiy
					};
					pRenderTarget->CreateBitmap(
						size,
						mImage->data(),
						mImage->numberOfBytesPerRow(), 
						&bitmapProps,
						&bitmap);
					D2D1_RECT_F frame = D2D1::RectF(0, 0, size.width, size.height);
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Matrix3x2F(1.f, 0, 0, -1.f, 0, size.height));
					pRenderTarget->DrawBitmap(
						bitmap,
						frame,
						1.0f,
						D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
						frame);

					hr = pRenderTarget->EndDraw();
					if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
					{
						discardGraphicsResources();
					}

					EndPaint(hwnd, &ps);
				}
				
				return 0;
			}

			default:
			{
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		}
		return TRUE;
	}

};

int main()
{
	RECT windowBounds = { 0,0,800,800 };
	AdjustWindowRectEx(&windowBounds, WS_OVERLAPPEDWINDOW, FALSE, 0);
	Window win;
	if (!win.createWindow(L"Ray Tracer", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, windowBounds.right - windowBounds.left, windowBounds.bottom - windowBounds.top))
	{
		return 0;
	}

	win.show();

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}