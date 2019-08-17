
#include "Game.h"

#include <Windows.h>
#include <dsound.h>
#include <Xinput.h>
#include <cmath>

#include <stdio.h>

#include "Win32.h"


#define WIDTH 1280
#define HEIGHT 720

struct win32_game_code
{
	HMODULE GameCodeDLL;

	// Note Either of these function pointers can be null
	// ensure to check for these before calling 
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	bool isValid;

	FILETIME lastWriteTime;
};

struct win32_buffer {
	int bpp;
	int width;
	int height;
	BITMAPINFO bitmapInfo;
	void* memoryBits;
	int pitch;
};
struct win32_dimension {
	int width;
	int height;
};
struct win32_sound_properties {
	int32_t samplesPerSecond;
	int32_t bytesPerSample;
	int32_t nChannels;
	int32_t bufferSize;
	int32_t runningSamples;
	int32_t latencySamples;
};


global_variable bool Running;
global_variable win32_buffer globalBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSoundBuffer;
global_variable LARGE_INTEGER globalPerfCountFrequency;
global_variable WINDOWPLACEMENT WindowPosition = { sizeof(WindowPosition) };

void ToggleFullScreen(HWND windowHandle)
{
 	DWORD Style = GetWindowLong(windowHandle, GWL_STYLE);
 	if (Style & WS_OVERLAPPEDWINDOW) {
    	MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
    	if (GetWindowPlacement(windowHandle, &WindowPosition) && GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
     		SetWindowLong(windowHandle, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
     		SetWindowPos(windowHandle, HWND_TOP,
                   MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                   MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                   MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    	}
 	} else{
    	SetWindowLong(windowHandle, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
    	SetWindowPlacement(windowHandle, &WindowPosition);
    	SetWindowPos(windowHandle, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  	}
}

debug_read_file_result DEBUGPlatformReadEntireFile(thread_context *Context, const char *filename){
	HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	ASSERT(handle != INVALID_HANDLE_VALUE);

	debug_read_file_result Result = {};
	void *content = nullptr;

	if (handle != INVALID_HANDLE_VALUE) {

		LARGE_INTEGER fileSize;

		if (GetFileSizeEx(handle, &fileSize)) {
			DWORD bytesRead;
			content = VirtualAlloc(NULL, fileSize.QuadPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			// content = (void*)new uint8[fileSize.QuadPart];
			// memset(content, 0, fileSize.QuadPart);

			ASSERT(content);

			DWORD size = truncateUint64(fileSize.QuadPart);

			if (ReadFile(handle, content, size, &bytesRead, NULL)) {
				ASSERT(size == bytesRead)
				Result.contents = content;
				Result.size = bytesRead;
			}else{
				uint32 Error = GetLastError();
				char messageBuffer[512];
				
				int success = FormatMessage( 
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					Error,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &messageBuffer,
					512, NULL );

				OutputDebugString(messageBuffer);
			}
		}
		CloseHandle(handle);
	}
	ASSERT(content);
	return Result;
}
void DEBUGPlatformFreeFileMemory(thread_context *Context, const debug_read_file_result* file_info){

	if (!VirtualFree(file_info->contents, file_info->size, MEM_RELEASE))
	{
		ASSERT(false);
	}

}
bool DEBUGPlatformWriteEntireFile(thread_context *Context, char *filename, uint32_t size, void *memory){
	HANDLE handle = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, NULL, NULL);

	bool result = false;

	if (handle != INVALID_HANDLE_VALUE) {

		DWORD bytesWritten;

		if (WriteFile(handle, memory, size, &bytesWritten, NULL)) {
			ASSERT(size == bytesWritten);
			result = true;
		}
		CloseHandle(handle);
	}

	return result;

}

internal void Win32ResizeDIBSection(win32_buffer *buffer, int width, int height) {

	buffer->bpp = 4;
	buffer->width = width;
	buffer->height = height;
	buffer->pitch = (width * buffer->bpp);

	BITMAPINFOHEADER bmiHeader = {};
	BITMAPINFO bitmapInfo = {};

	bmiHeader.biSize = sizeof(bitmapInfo);
	bmiHeader.biWidth = width;
	bmiHeader.biHeight = -height;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 32;
	bmiHeader.biCompression = BI_RGB;
	bmiHeader.biSizeImage = width * height * buffer->bpp;
	bmiHeader.biXPelsPerMeter;
	bmiHeader.biYPelsPerMeter;
	bmiHeader.biClrUsed;
	bmiHeader.biClrImportant;
	bitmapInfo.bmiHeader = bmiHeader;

	buffer->bitmapInfo = bitmapInfo;

	int bitmapMemorySize = width * height * buffer->bpp;
	buffer->memoryBits = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal win32_dimension Win32GetWindowSize(HWND handle) {
	RECT rect;
	GetClientRect(handle, &rect);
	return { rect.right - rect.left, rect.bottom - rect.top };
}

internal void Win32UpdateWindow(HDC deviceContext, win32_buffer *buffer, int width, int height) {
	// NOTE: For prototyping purposes we will blit 1-to-1 pixels to make sure we don't 
	// introduce artifacts stretching
	StretchDIBits(deviceContext,
		0, 0, buffer->width, buffer->height,
		0, 0, buffer->width, buffer->height, buffer->memoryBits, &buffer->bitmapInfo,
		DIB_RGB_COLORS, SRCCOPY);
}

inline LARGE_INTEGER Win32GetWallClock() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline float GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
	float result = (end.QuadPart - start.QuadPart) / (float)globalPerfCountFrequency.QuadPart;
	return result;
}

LRESULT CALLBACK Win32MainWindowCallback(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	
	switch (message) {
	case WM_SIZE:
	{
		OutputDebugStringA("WM_SIZE\n");
	}break;
	
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:{
		// Keyboard Message Shouldn't Come Through Here
		ASSERT(FALSE);
	}break;
	case WM_CLOSE: {
		Running = false;
		//TODO: Handle this with a message to the user
	}break;
	  
	case WM_ACTIVATEAPP: {
		//if(wParam == TRUE){
		//	SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 255, LWA_ALPHA);
		//}else{
		//	SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), 128, LWA_ALPHA);
		//}
	}break;
	case WM_DESTROY: {
		//TODO: Handle this as an error - recreate window
		Running = false;
	}break;
	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(windowHandle, &paint);
		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;
		int width = paint.rcPaint.right - paint.rcPaint.left;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;
		Win32UpdateWindow(deviceContext, &globalBuffer, width, height);
		EndPaint(windowHandle, &paint);
	}break;
	default:
		result = DefWindowProcA(windowHandle, message, wParam, lParam);
		break;
	}
	return result;
}

int StrLen(const char *str)
{
	int count = 0;
	while (*str++ != '\n')
	{
		count++;
	}
	return count;
}
void CatStrings(size_t AFileSize, const char *AFile, size_t BFileSize, const char *BFile, char *buffer)
{
	for (int i = 0; i < AFileSize; i++)
	{
		*buffer++ = *AFile++;
	}
	for (int i = 0; i < BFileSize; i++)
	{
		*buffer++ = *BFile++;
	}
}

void Win32GetEXEFileName(win32_state *win32State)
{
	GetModuleFileNameA(NULL, win32State->EXEFileName, sizeof(win32State->EXEFileName));
	char *onePastLastSlash = win32State->EXEFileName;
	for (char *scan = win32State->EXEFileName; *scan; scan++)
	{
		if (*scan == '\\')
		{
			win32State->OnePastLastEXEFileNameSlash = scan + 1;
		}
	}
}

void Win32BuildEXEPathFileName(win32_state *win32State, const char *filename, char *dest)
{
	size_t pathSize = win32State->OnePastLastEXEFileNameSlash - win32State->EXEFileName;
	CatStrings(pathSize, win32State->EXEFileName, StrLen(filename), filename, dest);
}

static void Win32BeginRecordingInput(win32_state *win32State, int RecordingIndex){
	char fullPath[WIN32_MAX_PATH];
	Win32BuildEXEPathFileName(win32State, win32State->filename, fullPath);
	HANDLE handle = CreateFileA(fullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	ASSERT(handle != INVALID_HANDLE_VALUE);

	DWORD bytesReturned;
	DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesReturned, NULL);
	win32State->RecordingHandle = handle;

	HANDLE gameStateHandle = CreateFileA("game_state", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	DWORD bytesWritten;
	WriteFile(gameStateHandle, win32State->gameMemoryBlock, win32State->totalSize, &bytesWritten, NULL);
	ASSERT(win32State->totalSize == bytesWritten);
	CloseHandle(gameStateHandle);
}

static void Win32EndRecordingInput(win32_state *win32State){
	ASSERT(CloseHandle(win32State->RecordingHandle));
}

static void Win32BeginInputPlayback(win32_state *win32State, int PlaybackIndex){
	char fullPath[WIN32_MAX_PATH];
	Win32BuildEXEPathFileName(win32State, win32State->filename, fullPath);
	HANDLE handle = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	ASSERT(handle != INVALID_HANDLE_VALUE);
	win32State->PlaybackHandle = handle;

	HANDLE gameStateHandle = CreateFileA("game_state", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	DWORD bytesRead;
	ReadFile(gameStateHandle, win32State->gameMemoryBlock, win32State->totalSize, &bytesRead, NULL);
	ASSERT(win32State->totalSize == bytesRead);
	CloseHandle(gameStateHandle);

}

static void Win32EndInputPlayback(win32_state *win32State){
	CloseHandle(win32State->PlaybackHandle);
}

static void Win32RecordInput(win32_state *win32State, game_input *newInput){
	DWORD bytesWritten;
	WriteFile(win32State->RecordingHandle, newInput, sizeof(*newInput), &bytesWritten, NULL);
}

static void Win32PlaybackInput(win32_state *win32State, game_input *newInput){
	DWORD bytesRead;
	if(ReadFile(win32State->PlaybackHandle, newInput, sizeof(*newInput), &bytesRead, NULL)){
		if (bytesRead == 0)
		{
			Win32EndInputPlayback(win32State);
			Win32BeginInputPlayback(win32State, win32State->InputPlaybackIndex);
			ReadFile(win32State->PlaybackHandle, newInput, sizeof(*newInput), &bytesRead, NULL);
		}
		ASSERT(bytesRead == sizeof(*newInput));
	}else{
		ASSERT(false);
	}
}

#define XINPUT_GET_STATE(name) DWORD name( DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(x_input_get_state);
XINPUT_GET_STATE(XInputGetStateStub) { return 1; }
x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef XINPUT_SET_STATE(x_input_set_state);
XINPUT_SET_STATE(XInputSetStateStub) { return 1; }
x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter )
typedef DIRECT_SOUND_CREATE(direct_sound_create);
DIRECT_SOUND_CREATE(DirectSoundCreateStub) { return 1; }
direct_sound_create* DirectSoundCreate_ = DirectSoundCreateStub;
#define DirectSoundCreate DirectSoundCreate_

FILETIME Win32GetLastWriteTime(const char *filename)
{
	WIN32_FILE_ATTRIBUTE_DATA fileData;
	ASSERT(GetFileAttributesExA(filename, GetFileExInfoStandard, &fileData));

	return fileData.ftLastWriteTime;
}

win32_game_code Win32LoadGameCode(const char* sourceDllName, const char* tempDllName)
{
	win32_game_code Result = {};

	CopyFileA(sourceDllName, tempDllName, FALSE);
	Result.lastWriteTime = Win32GetLastWriteTime(sourceDllName);
	Result.GameCodeDLL = LoadLibraryA(tempDllName);


	if (Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

		Result.isValid = (Result.UpdateAndRender && Result.GetSoundSamples);
	}

	if (!Result.isValid)
	{
		Result.UpdateAndRender = NULL;
		Result.GetSoundSamples = NULL;
	}

	return Result;
}

void Win32UnloadGameCode(win32_game_code *GameCode){
	FreeLibrary(GameCode->GameCodeDLL);
}

internal void Win32LoadXInput()
{

	HMODULE XinputLibrary = LoadLibraryA("xinput1_3.dll");
	if (!XinputLibrary) {
		XinputLibrary = LoadLibraryA("xinput9_1.dll");
	}
	if (!XinputLibrary) {
		XinputLibrary = LoadLibraryA("xinput1_4.dll");
	}
	if (XinputLibrary) {
		XInputGetState = (x_input_get_state*)GetProcAddress(XinputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XinputLibrary, "XInputSetState");
	}
}

internal void Win32LoadDSound(HWND windowHandle, uint32_t bufferSize) {

	HRESULT result;

	HMODULE library = LoadLibraryA("dsound.dll");
	if (library) {
		direct_sound_create *func = (direct_sound_create*)GetProcAddress(library, "DirectSoundCreate8");
		if (func) {
			DirectSoundCreate = func;
		}
		else {
			OutputDebugStringA("Failed to locate dsound.dll");
			__debugbreak();
		}

		LPDIRECTSOUND DirectSound;

		result = DirectSoundCreate(NULL, &DirectSound, NULL);
		if (SUCCEEDED(result)) {
			WAVEFORMATEX waveformat;
			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = 2;
			waveformat.nSamplesPerSec = 48000;
			waveformat.wBitsPerSample = 16;
			waveformat.nBlockAlign = (waveformat.nChannels * waveformat.wBitsPerSample) / 8;
			waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign; //Same as nSamplesPerSec * nChannels * sizeof(BitsPerSample);
			waveformat.cbSize = 0;


			if (SUCCEEDED(DirectSound->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY))) {

				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER primaryBuffer;
				result = DirectSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, NULL);
				if (SUCCEEDED(result)) {

					result = primaryBuffer->SetFormat(&waveformat);
					if (SUCCEEDED(result)) {
						OutputDebugStringA("Primary buffer was set");
					}
					else {
						//TODO: Diagnostic
						__debugbreak();
						OutputDebugStringA("Failed to set buffer format");
					}
				}
				else {
					__debugbreak();
					//TODO: Diagnostic
				}
			}
			else {
				__debugbreak();
				//TODO: Diagnostic
			}

			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags;
			bufferDescription.lpwfxFormat = &waveformat;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.dwReserved = 0;


			result = DirectSound->CreateSoundBuffer(&bufferDescription, &globalSoundBuffer, NULL);
			if (SUCCEEDED(result)) {
				OutputDebugStringA("Second buffer created successfully");
			}
			else {
				__debugbreak();
				//TODO: Diagnostic
			}
		}
	}
}

internal void Win32FillSoundBuffer(win32_sound_properties *sound_properties, int32_t byteToLock, int32_t bytesToWrite, game_sound_buffer *sourceBuffer) {

	VOID *region1;
	DWORD region1BytesSize;
	VOID *region2;
	DWORD region2BytesSize;

	HRESULT result = globalSoundBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1BytesSize, &region2, &region2BytesSize, NULL);
	bool32 succeeded = SUCCEEDED(result);
	if (succeeded) {

		int16_t* dest = (int16_t*)region1;
		int16_t* src = (int16_t*)sourceBuffer->samples;

		for (int i = 0; i < region1BytesSize / sound_properties->bytesPerSample; i++) {
			*dest++ = *src++;
			//sound_properties->runningSamples++;
		}

		dest = (int16_t*)region2;
		for (int i = 0; i < region2BytesSize / sound_properties->bytesPerSample; i++) {
			*dest++ = *src++;
			//sound_properties->runningSamples++;
		}
		globalSoundBuffer->Unlock(region1, region1BytesSize, region2, region2BytesSize);
	}
	else {
		//TODO: Diagnostics
	}

}

internal void Win32ClearSoundBuffer(win32_sound_properties* sound_properties) {

	VOID *region1;
	DWORD region1BytesSize;
	VOID *region2;
	DWORD region2BytesSize;

	HRESULT result = globalSoundBuffer->Lock(0, sound_properties->bufferSize, &region1, &region1BytesSize, &region2, &region2BytesSize, NULL);
	if (result == DS_OK) {
		int8_t *samples = (int8_t*)region1;
		for (int i = 0; i < region1BytesSize; i++) {
			*samples++ = 0;
		}
		samples = (int8_t*)region2;
		for (int i = 0; i < region2BytesSize; i++) {
			*samples++ = 0;
		}

		globalSoundBuffer->Unlock(region1, region1BytesSize, region2, region2BytesSize);
	}

}

static void Win32ProcessKeyboardMessage(game_button_state *newState, bool isDown){
	//ASSERT(newState->endedDown != isDown);
	newState->endedDown = isDown;
	newState->halfTransitionCount++;
}

static void Win32ProcessPendingMessages(game_controller_input *keyboardController, win32_state *win32State){
	MSG message;
	while(PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE)){
		switch (message.message)
		{
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:{

			bool wasDown = (message.lParam & (1 << 30));
			bool isDown = (message.lParam & (1 << 31)) == 0;
			uint32_t VKCode = (uint32_t)message.wParam;

			if (wasDown != isDown)
			{
				if(VKCode == 'W'){
					Win32ProcessKeyboardMessage(&keyboardController->moveUp, isDown);
				}
				if(VKCode == 'A'){
					Win32ProcessKeyboardMessage(&keyboardController->moveLeft, isDown);
				}
				if(VKCode == 'S'){
					Win32ProcessKeyboardMessage(&keyboardController->moveDown, isDown);
				}
				if(VKCode == 'D'){
					Win32ProcessKeyboardMessage(&keyboardController->moveRight, isDown);
				}
				if(VKCode == VK_SPACE){
					Win32ProcessKeyboardMessage(&keyboardController->start, isDown);
				}
				if(VKCode == 'L'){
					if(isDown){
						OutputDebugStringA("L\n");
						if(!win32State->InputRecordingIndex){
							OutputDebugStringA("Stop Playback\n");
							win32State->InputPlaybackIndex = 0;
							Win32EndInputPlayback(win32State);
							OutputDebugStringA("Begin Recording\n");
							win32State->InputRecordingIndex = 1;
							Win32BeginRecordingInput(win32State, win32State->InputRecordingIndex);
							game_controller_input zeroController = {};
							*keyboardController = zeroController;
						}else{
							win32State->InputRecordingIndex = 0;
							win32State->InputPlaybackIndex = 1;
							OutputDebugStringA("Stop Recording\n");
							Win32EndRecordingInput(win32State);
							OutputDebugStringA("Begin Playback\n");
							Win32BeginInputPlayback(win32State, win32State->InputPlaybackIndex);
						}
					}
				}
				bool AltKeyWasDown = (message.lParam & (1 << 29));
				if((VKCode == VK_RETURN) && (AltKeyWasDown)){
					if(isDown){
						ToggleFullScreen(message.hwnd);
					}
				}
			}
		}
		break;
		default:
			TranslateMessage(&message);
			DispatchMessage(&message);
			break;
		}
		
	}
}

internal void Win32ProcessXinputButton(WORD xinputState, DWORD ButtonBit, game_button_state *oldState, game_button_state *newState)
{
	newState->endedDown = (xinputState & ButtonBit);
	newState->halfTransitionCount = (newState->endedDown != oldState->endedDown) ? 1 : 0;
}

internal float Win32ProcessXinputStickValue(short thumbStickValue, int deadzone) {
	float x = 0;
	if (thumbStickValue < 0 - deadzone) {
		x = thumbStickValue / 32768.f;
	}
	else if (thumbStickValue > deadzone) {
		x = thumbStickValue / 32767.f;
	}
	return x;
}

inline void Win32DrawRect(win32_buffer *buffer, int32_t x, int32_t y, int32_t width, int32_t height) {

	ASSERT( (WIDTH - x) >= width);
	ASSERT( (HEIGHT - y) >= height);

	int8_t* row = ((int8_t*)buffer->memoryBits + (x * 4));
	row += (buffer->pitch * y);

	for (int i = 0; i < height; i++) {
		int32_t *pixel = (int32_t*)row;
		for (int k = 0; k < width; k++) {
			int32_t value = 0xFFFF00;
			*pixel = value;
			pixel++;
		}
		row += buffer->pitch;
	}
}

internal void Win32DebugSyncDisplay(win32_buffer *buffer, int16_t *sample, uint32_t sampleSize) {

	int block_size = sampleSize / 2;

	int width = ceil((buffer->width - 64) / (float)block_size);

	float c = (buffer->width - 64) / (float)block_size;

	for (int i = 0; i < block_size; i++) {
		float offset = (i * c) + 32;
		int32_t *start = (int32_t*)buffer->memoryBits;
		start += (int)offset;
		float height = (*sample + 32768) * (buffer->height - 32) / 0xFFFF;
		//DrawRect(start, width, height);
		sample += 2;
	}
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow) {

	HRESULT result;
	win32_state win32State = {};
	Win32GetEXEFileName(&win32State);
	char sourceDllFullPath[MAX_PATH];
	Win32BuildEXEPathFileName(&win32State, "Game.dll", sourceDllFullPath);
	char tempDllFullPath[MAX_PATH];
	Win32BuildEXEPathFileName(&win32State, "Game_Temp.dll", tempDllFullPath);

	//Set Windows Sleep Granularity to 1ms
	UINT desiredScheduler = 1;
	bool sleepIsGranular = timeBeginPeriod(desiredScheduler) == TIMERR_NOERROR;
	const float targetSecondsPerFrame = 1 / 30.0;

	//TODO: Check if HREDRAW AND VREDRAW still necessary 
	WNDCLASSA windowClass = {};

	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.style = /*CS_OWNDC|*/ CS_HREDRAW | CS_VREDRAW;
	windowClass.lpszClassName = "TileGameDemoClass";
	windowClass.hCursor = LoadCursor(0, IDC_CROSS);

	ATOM status  = RegisterClassA(&windowClass);
	ASSERT(status);

	HWND windowHandle = CreateWindowExA(
		NULL /* WS_EX_LAYERED */,
		windowClass.lpszClassName,
		"TileGame Demo",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WIDTH,
		HEIGHT,
		0,
		0,
		hInstance,
		0);

	ASSERT(windowHandle);

	ShowWindow(windowHandle, nCmdShow);

	win32_sound_properties sound = {};
	sound.samplesPerSecond = 48000;
	sound.bytesPerSample = sizeof(int16_t);
	sound.latencySamples = sound.samplesPerSecond / 15;
	sound.nChannels = 2;
	sound.runningSamples = 0;
	sound.bufferSize = (sound.samplesPerSecond * sound.bytesPerSample * sound.nChannels * 3);
	//Load Required Libraries
	Win32LoadDSound(windowHandle, sound.bufferSize);
	Win32LoadXInput();
	int16_t *samples = new int16_t[(sound.samplesPerSecond * sound.nChannels) * 3];
	Win32ClearSoundBuffer(&sound);
	globalSoundBuffer->Play(NULL, NULL, DSBPLAY_LOOPING);
	Running = true;

	
	//Initialize Setup or back_buffer
	Win32ResizeDIBSection(&globalBuffer, WIDTH, HEIGHT);

	int MonitorRefreshHz = 60;
	HDC refreshDC = GetDC(windowHandle);
	int Win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
	ReleaseDC(windowHandle, refreshDC);
	if (Win32RefreshRate > 1)
	{
		MonitorRefreshHz = Win32RefreshRate;
	}

	game_memory gameMemory = {};
	// Note: permanentStorageSize = 64MB
	gameMemory.permanentStorageSize = Megabytes(16);
	// Note: transientStorageSize = 4GB
	gameMemory.transientStorageSize = Megabytes((uint64_t)64);
	LPVOID baseAddress = 0;
	// TODO use LARGE_MEM_PAGES and call AdjustToken
	win32State.gameMemoryBlock = VirtualAlloc(baseAddress, gameMemory.permanentStorageSize + gameMemory.transientStorageSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	win32State.totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
	if (win32State.gameMemoryBlock == nullptr) {
		ASSERT(false);
	}
	gameMemory.permanentStorage = win32State.gameMemoryBlock;
	gameMemory.transientStorage = (uint8_t*)(win32State.gameMemoryBlock) + gameMemory.permanentStorageSize;

	gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
	gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
	gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

	game_state GameState = {};
	GameState.toneHz = 440;
	*(game_state*)(gameMemory.permanentStorage) = GameState;

	game_input input[2] = {};
	game_input *oldInput = &input[0];
	game_input *newInput = &input[1];
	QueryPerformanceFrequency(&globalPerfCountFrequency);
	LARGE_INTEGER lastCounter = Win32GetWallClock();
	win32_game_code GameCode = Win32LoadGameCode(sourceDllFullPath, tempDllFullPath);

	while(Running)
	{
		FILETIME newDllWriteTime = Win32GetLastWriteTime(sourceDllFullPath);
		if (CompareFileTime(&newDllWriteTime, &GameCode.lastWriteTime) != 0)
		{
			OutputDebugStringA(" >> Updating....\n");
			Win32UnloadGameCode(&GameCode);
			GameCode = Win32LoadGameCode(sourceDllFullPath, tempDllFullPath);
		}

		game_controller_input *oldKeyboardController = &oldInput->controllers[0];
		game_controller_input *newKeyboardController = &newInput->controllers[0];
		game_controller_input zeroController = {};
		*newKeyboardController = zeroController;
		for (int buttonIndex = 0; buttonIndex < ArrayCount(newKeyboardController->buttons);buttonIndex++)
		{
			newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[buttonIndex].endedDown;
		}
		Win32ProcessPendingMessages(newKeyboardController, &win32State);
		int maxControllerCount = XUSER_MAX_COUNT + 1;
		ASSERT(ArrayCount(newInput->controllers) == 5);
		if(maxControllerCount > ArrayCount(newInput->controllers)){
			maxControllerCount = ArrayCount(newInput->controllers);
		}

		for (DWORD i = 0; i< maxControllerCount-1; i++)
		{
			int controllerIndex = i + 1;
			game_controller_input *oldState = &(oldInput->controllers[controllerIndex]);
			game_controller_input *newState = &(newInput->controllers[controllerIndex]);
			XINPUT_STATE controllerState;
			if (XInputGetState(i, &controllerState) == ERROR_SUCCESS) {
				newState->isConnected = true;
				XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_DPAD_UP, &oldState->moveUp, &newState->moveUp);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN, &oldState->moveDown, &newState->moveDown);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT, &oldState->moveLeft, &newState->moveLeft);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, &oldState->moveRight, &newState->moveRight);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, &oldState->left_shoulder, &newState->left_shoulder);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, &oldState->right_shoulder, &newState->right_shoulder);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_START, &oldState->start, &newState->start);
				Win32ProcessXinputButton(gamepad->wButtons, XINPUT_GAMEPAD_BACK, &oldState->end, &newState->end);
				float x = Win32ProcessXinputStickValue(gamepad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				float y = Win32ProcessXinputStickValue(gamepad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				newState->stickAverageX = x;
				newState->stickAverageY = y;
				if((x != 0.0f) || (y != 0.0f)){
					newState->isAnalog = true;
				}
				if(newState->moveUp.endedDown){
					newState->isAnalog = false;
				}
				if(newState->moveDown.endedDown){
					newState->isAnalog = false;
				}
				if(newState->moveLeft.endedDown){
					newState->isAnalog = false;
				}
				if(newState->moveRight.endedDown){
					newState->isAnalog = false;
				}
			}
		}

		//DirectSound
		//DWORD playCursor;
		//DWORD writeCursor;
		//DWORD byteToLock = 0;
		//DWORD bytesToWrite = 0;
		//int latency;
		//local_persist bool playing;
		//if (SUCCEEDED(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
		//
		//
		//	//TODO: include block size in sound struct to make calculations easier
		//	byteToLock += (sound.samplesPerSecond * sound.nChannels * sound.bytesPerSample) % sound.bufferSize;
		//
		//
		//	bytesToWrite = sound.samplesPerSecond * sound.nChannels * sound.bytesPerSample;
		//
		//	latency = writeCursor - playCursor;
		//	//if (latency < 0) {
		//	//	latency = sound.bufferSize + latency;
		//	//}
		//	//
		//	//float latencyTime = (latency / (float)(sound.bytesPerSample * sound.nChannels)) / sound.samplesPerSecond;
		//	//float writeCursorTime = GetSecondsElapsed(lastCounter, Win32GetWallClock()) + latencyTime;
		//	//
		//	//
		//	//if (writeCursorTime < targetSecondsPerFrame) {
		//	//	bytesToWrite = sound.samplesPerSecond;
		//	//	byteToLock += (sound.samplesPerSecond) % sound.bufferSize;
		//	//}
		//	//else {
		//	//	OutputDebugStringA("Missed Frame");
		//	//}
		//	//
		//	//
		//	//
		//	//byteToLock = (sound.runningSamples * sound.bytesPerSample) % sound.bufferSize;
		//	//
		//	//if (byteToLock > playCursor) {
		//	//	bytesToWrite = sound.bufferSize - byteToLock;
		//	//	bytesToWrite += playCursor;
		//	//}
		//	//else {
		//	//	bytesToWrite = playCursor - byteToLock;
		//	//}
		//	//sound.runningSamples += bytesToWrite / sound.bytesPerSample;
		//}
		//else {
		//	//TODO: Diagnostics
		//	ASSERT(false);
		//}
		//
		//char buffer[512];
		//wsprintfA(buffer, "PC: %d WC:%d latency:%d\n", playCursor, writeCursor, latency);
		//OutputDebugStringA(buffer);
		//
		game_sound_buffer sound_buffer = {};
		//sound_buffer.sampleCount = bytesToWrite / sound.bytesPerSample;
		//sound_buffer.samples = samples;

		game_back_buffer back_buffer = {};
		back_buffer.bytesPerPixel = globalBuffer.bpp;
		back_buffer.height = globalBuffer.height;
		back_buffer.width = globalBuffer.width;
		back_buffer.memory = globalBuffer.memoryBits;
		back_buffer.pitch = globalBuffer.pitch;

		//Fill backbuffer and soundbuffer
		if(win32State.InputRecordingIndex){
			OutputDebugStringA("Recording...\n");
			Win32RecordInput(&win32State, newInput);
		}
		if(win32State.InputPlaybackIndex){
			OutputDebugStringA("Playbacking...\n");
			Win32PlaybackInput(&win32State, newInput);
		}
		if (GameCode.UpdateAndRender){
			GameCode.UpdateAndRender(nullptr, &gameMemory, newInput, &back_buffer);
		}
		if (GameCode.GetSoundSamples){
			//GameCode.GetSoundSamples(nullptr, &gameMemory, &sound_buffer);
		}	
		Win32DebugSyncDisplay(&globalBuffer, samples, sound.samplesPerSecond * 3);
		//Win32FillSoundBuffer(&sound, byteToLock, bytesToWrite, &sound_buffer);

		// TIMING 
		float elapsed = GetSecondsElapsed(lastCounter, Win32GetWallClock());
		while (elapsed < targetSecondsPerFrame) {
			if (sleepIsGranular) {
				DWORD sleepTime = (DWORD) ((targetSecondsPerFrame - elapsed) * 1000.f);
					Sleep(sleepTime);
				}
				elapsed = GetSecondsElapsed(lastCounter, Win32GetWallClock());
			}
		lastCounter = Win32GetWallClock();
		newInput->dt = elapsed;
		char timing[512];
		sprintf(timing, "%.6f s\n", elapsed);
		OutputDebugStringA(timing);

		//Update Display
		HDC context = GetDC(windowHandle);
		win32_dimension dimensions = Win32GetWindowSize(windowHandle);
		Win32UpdateWindow(context, &globalBuffer, dimensions.width, dimensions.height);
		ReleaseDC(windowHandle, context);
		*oldInput = *newInput;
	}
	delete[] samples;
	return 0;
}
