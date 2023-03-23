#include "Game.h"

Game* Game::instance = nullptr;

/*
* Static method for handling user input
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
	return Game::instance->MessageHandler(hwnd, umessage, wparam, lparam);
}

LRESULT Game::MessageHandler(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
	switch (umessage) {
	case WM_MOUSEMOVE: {
		const POINTS pt = MAKEPOINTS(lparam);
		mouseY = pt.y;
	}
	case WM_INPUT: {
		UINT dwSize = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
		LPBYTE lpb = new BYTE[dwSize];

		if (lpb == nullptr)
			return 0;

		if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

		RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);

		if (raw->header.dwType == RIM_TYPEKEYBOARD) {

			inputDevice.get()->OnKeyDown({
				raw->data.keyboard.MakeCode,
				raw->data.keyboard.Flags,
				raw->data.keyboard.VKey,
				raw->data.keyboard.Message
				});
		}
		else if (raw->header.dwType == RIM_TYPEMOUSE) {
			
			inputDevice->OnMouseMove({
				raw->data.mouse.usFlags,
				raw->data.mouse.usButtonFlags,
				static_cast<int>(raw->data.mouse.ulExtraInformation),
				static_cast<int>(raw->data.mouse.ulRawButtons),
				static_cast<short>(raw->data.mouse.usButtonData),
				raw->data.mouse.lLastX,
				raw->data.mouse.lLastY
				});
		}

		delete[] lpb;
		return DefWindowProc(hwnd, umessage, wparam, lparam);
	}
	default: {
		return DefWindowProc(hwnd, umessage, wparam, lparam);
	}
	}
}

/* Singleton method*/
Game::Game(LPCWSTR name, int screenWidth, int screenHeight) {
	this->name = name;
	this->clientWidth = screenWidth;
	this->clientHeight = screenHeight;

	totalTime = 0;
	deltaTime = 0;
	frameCount = 0;
	startTime = new std::chrono::time_point<std::chrono::steady_clock>();
	prevTime = new std::chrono::time_point<std::chrono::steady_clock>();
}

void Game::CreateInstance(LPCWSTR name, int screenWidth, int screenHeight) {
	if (!instance)
		instance = new Game(name, screenWidth, screenHeight);
}

void Game::CreateBackBuffer() {

}

/*
* Method for preparing all "Game" resources
*/
void Game::PrepareResources() {
	inputDevice = std::make_shared<InputDevice>();

	const int featureLevelsNumber = 1;
	D3D_FEATURE_LEVEL featureLevels[featureLevelsNumber] = { D3D_FEATURE_LEVEL_11_1 };

	swapDesc = {}; 
	
	swapDesc.BufferCount = 2; 
	swapDesc.BufferDesc.Width = display->GetScreenWidth();
	swapDesc.BufferDesc.Height = display->GetScreenHeight(); 
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
	swapDesc.BufferDesc.RefreshRate.Numerator = 60; 
	swapDesc.BufferDesc.RefreshRate.Denominator = 1; 
	swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; 
	swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	swapDesc.OutputWindow = display->GetHWnd(); 
	swapDesc.Windowed = true;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; 
	swapDesc.SampleDesc.Count = 1; 
	swapDesc.SampleDesc.Quality = 0; 

	
	auto res = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevels, 
		featureLevelsNumber,
		D3D11_SDK_VERSION,
		&swapDesc,
		&swapChain,
		&device,
		nullptr, 
		&context);

	if (FAILED(res)) {
		// Well, that was unexpected
	}

	res = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backTex.GetAddressOf()); // Accesses one of the buffers of the back buffer chain
	res = device->CreateRenderTargetView(backTex.Get(), nullptr, rtv.GetAddressOf());

	viewport = {};
	viewport.Width = static_cast<float>(display->GetScreenWidth());
	viewport.Height = static_cast<float>(display->GetScreenHeight());
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;
}

/*
* Method for initializing all "GameComponent" items in vector
*/
void Game::Initialize() {
	// Compile shaders
	for (auto component : components)
		component->Initialize();
}

void Game::PrepareFrame() {
	context->ClearState(); // Reset parameters to default

	context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

	context->RSSetViewports(1, &viewport);

	float color[] = { 0.0f, 0.1f, 0.1f, 1.0f }; //background color
	context->ClearRenderTargetView(rtv.Get(), color);
}

/*
* Method for updating all "GameComponent" items in vector
*/
void Game::Update() {
	for (auto component : components)
		component->Update();
	for (auto component : components2) {
		component->Initialize();
		components.push_back(component);
	}
	components2.clear();
}

/*
* Method for drawing all "GameComponent" items in vector
*/
void Game::Draw() {
	for (auto component : components)
		component->Draw();
}

void Game::RestoreTargets() {

}

/*
* Method for actually presenting graphics
*/
void Game::EndFrame() {
	context->OMSetRenderTargets(0, nullptr, nullptr);

	swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0); // Show what we've drawn
}

/*
* Internal method, that updates all items and called each frame
*/
void Game::UpdateInternal() {
	auto curTime = std::chrono::steady_clock::now();
	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - *prevTime).count() / 1000000.0f;
	*prevTime = curTime;

	totalTime += deltaTime;
	frameCount++;

	if (totalTime > 1.0f) {
		float fps = frameCount / totalTime;

		totalTime -= 1.0f;

		WCHAR text[256];
		swprintf_s(text, TEXT("FPS: %f"), fps);
		//SetWindowText(display->GetHWnd(), text);

		frameCount = 0;
	}

	PrepareFrame();

	Update();

	Draw();

	RestoreTargets();

	EndFrame();
}

void Game::Run() {
	display = std::make_shared<DisplayWin32>(name, clientWidth, clientHeight, WndProc);

	PrepareResources();

	Initialize();

	isExitRequested = false;
	
	*startTime = std::chrono::steady_clock::now();
	*prevTime = *startTime;
	totalTime = 0;
	frameCount = 0;

	MSG msg = {};
	while (!isExitRequested) {
		// Handle the windows messages.
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT) {
			isExitRequested = true;
		}

		UpdateInternal();
	}

	delete startTime;
	delete prevTime;
	DestroyResources();
}

void Game::Exit() {

}

void Game::DestroyResources() {
	for (auto component : components)
		component->DestroyResources();
}


std::shared_ptr<DisplayWin32> Game::GetDisplay() {
	return display;
}

Microsoft::WRL::ComPtr<ID3D11Device> Game::GetDevice() {
	return device;
}

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> Game::GetRTV() {
	return rtv;
}

Microsoft::WRL::ComPtr <IDXGISwapChain> Game::GetSwapChain() {
	return swapChain;
}

Microsoft::WRL::ComPtr <ID3D11DeviceContext> Game::GetContext() {
	return context;
}

float Game::GetTotalTime() {
	return totalTime;
}

void Game::SetTotalTime(float totalTime) {
	this->totalTime = totalTime;
}

unsigned int Game::GetFrameCount() {
	return frameCount;
}

void Game::SetFrameCount(unsigned int frameCount) {
	this->frameCount = frameCount;
}
