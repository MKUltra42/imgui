// ImGui - standalone example application for DirectX 11
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

void CreateRenderTarget()
{
    DXGI_SWAP_CHAIN_DESC sd;
    g_pSwapChain->GetDesc(&sd);

    // Create the render target
    ID3D11Texture2D* pBackBuffer;
    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
    ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
    render_target_view_desc.Format = sd.BufferDesc.Format;
    render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return E_FAIL;

    CreateRenderTarget();

    return S_OK;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

#include <vector>
#include <algorithm>

ImVec2 g_offset;
const float GRID_SZ = 64.0f;


struct Key
{
    float   time = 0.0f;
    float   val = 0.0f;
    bool    selected = false;

    Key(float _time, float _val) : time(_time), val(_val) {}
};

struct Track
{
    const char*         name;
    std::vector<Key>    keys;
    ImColor             col;
    bool                visible = true;

    Track(const char* _name, const std::vector<Key>& _keys, ImColor _col) : name(_name), keys(_keys), col(_col) {}
};


ImVec2 KeyToTimeLine(const Key& key)
{
    return g_offset + ImVec2(key.time * GRID_SZ/0.25f, key.val * -GRID_SZ + 5 * GRID_SZ);
}

Key TimeLineToKey(ImVec2 pos)
{
    //pos = pos - g_offset;
    return Key(pos.x * 0.25f/GRID_SZ, (pos.y) / -GRID_SZ);
}

bool IsPointInside(const ImVec2& corner1, const ImVec2& corner2, const ImVec2& pt, float border = 0.0f)
{
    ImVec2 a = ImVec2(std::min<>(corner1.x-border, corner2.x-border), std::min<>(corner1.y-border, corner2.y-border));
    ImVec2 b = ImVec2(std::max<>(corner1.x+border, corner2.x+border), std::max<>(corner1.y+border, corner2.y+border));

    return (a.x < pt.x && b.x > pt.x && a.y < pt.y && b.y > pt.y);
}

// Really dumb data structure provided for the example.
// Note that we storing links are INDICES (not ID) to make example code shorter, obviously a bad idea for any general purpose code.
static void ShowExampleAppCurveEditor(bool* opened)
{
	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("CurveEditor", opened))
	{
		ImGui::End();
		return;
	}

    static bool inited = false;
    static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    static bool moving = false;
    static ImVec2 moveBegin = ImVec2(0.0f, 0.0f);
    static bool selecting = false;
    static ImVec2 dragBegin = ImVec2(0.0f, 0.0f);
    static ImVec2 dragEnd = ImVec2(0.0f, 0.0f);
    static bool show_grid = true;
	static int node_selected = -1;

    static std::vector<Track> s_tracks;

    const ImColor BACKGROUND_COL = ImColor(30, 30, 30, 255);
    const ImColor KEY_COL = ImColor(221, 109, 64);

    if (!inited)
	{
        s_tracks.push_back(Track("Red", { Key(0, 1.0f), Key(1, 0.0f), Key(2, 0.2f)}, ImColor(255, 0, 0)));
        s_tracks.push_back(Track("Green", { Key(0, 0.5f), Key(1, 0.6f), Key(2, 0.0f) }, ImColor(0, 255, 0)));
        s_tracks.push_back(Track("Blue", { Key(0, 0.0f), Key(1, 0.4f), Key(2, 1.0f) }, ImColor(0, 0, 255)));
        inited = true;
	}

    std::vector<Track> newTracks;

    for (int t = 0; t < s_tracks.size(); ++t)
    {
        Track& track = s_tracks[t];
        Track newTrack(track.name, {}, track.col);
        newTrack.visible = track.visible;

        for (int k = 0; k < track.keys.size(); ++k)
        {
            Key& key = track.keys[k];
            Key newKey = key;
            newKey.selected = key.selected;

            if (moving && newKey.selected)
            {
                Key offset = TimeLineToKey(ImGui::GetMousePos() - moveBegin);
                newKey.time += offset.time;
                newKey.val += offset.val;
            }

            newTrack.keys.push_back(newKey);
        }

        std::sort(newTrack.keys.begin(), newTrack.keys.end(), [](Key a, Key b)
        {
            return b.time < a.time;
        });

        newTracks.push_back(newTrack);
    }

    std::vector<Track>& tracks = (moving) ? newTracks : s_tracks;

	// Draw a list of nodes on the left side
	bool open_context_menu = false;
	int node_hovered_in_list = -1;
	int node_hovered_in_scene = -1;
	ImGui::BeginChild("track_list", ImVec2(100, 0));
	ImGui::Text("Tracks");
	ImGui::Separator();

    for (int i = 0; i < tracks.size(); ++i)
    {
        ImGui::Checkbox(tracks[i].name, &tracks[i].visible);
    }

    ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	const float NODE_SLOT_RADIUS = 4.0f;
	const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

	// Create our child canvas
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60, 60, 60, 255));


	ImGui::BeginChild("timeline", ImVec2(0, 20), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImU32 CURSOR_LINE_COLOR = ImColor(250, 249, 5, 255);

	bool doLine = false;

	if (show_grid)
	{
		//ImU32 GRID_COLOR = ImColor(200, 200, 200, 40);
		ImU32 TEXT_COLOR = ImColor(255, 255, 255, 220);
		ImU32 MAJOR_TICK_COLOR = ImColor(200, 200, 200, 140);
		ImU32 MINOR_TICK_COLOR = ImColor(200, 200, 200, 40);
		ImVec2 win_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_sz = ImGui::GetWindowSize();
		for (float x = fmodf(-scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
		{
			char buff[100];
			sprintf_s(buff, sizeof(buff), "%.2fs", (x+scrolling.x)/GRID_SZ * 0.25f);
			draw_list->AddText(ImVec2(x + 2.0f, 0.0f) + win_pos, TEXT_COLOR, buff);
			draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, MAJOR_TICK_COLOR);

			float subdiv = 1.0f / 5.0f;
			for (float sx = subdiv; sx < 0.9999f; sx += subdiv)
			{
				draw_list->AddLine(ImVec2(x + sx*GRID_SZ, 15.0f) + win_pos, ImVec2(x + sx*GRID_SZ, canvas_sz.y) + win_pos, MINOR_TICK_COLOR);
			}
		}

		if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
		{
			doLine = true;
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, BACKGROUND_COL);
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	g_offset = ImGui::GetCursorScreenPos() - scrolling;

	draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);

	// Display grid
	if (show_grid)
	{
		ImU32 GRID_COLOR = ImColor(200, 200, 200, 40);
		ImVec2 win_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_sz = ImGui::GetWindowSize();
		for (float x = fmodf(-scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
			draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
		for (float y = fmodf(-scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
			draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);

		if (doLine || (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive()))
		{
			//float x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
			//draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, CURSOR_LINE_COLOR);
		}
	}

	// Display links
	draw_list->ChannelsSetCurrent(0); // Background

    for (int t = 0; t < tracks.size(); ++t)
    {
        const Track& track = tracks[t];

        if (!track.visible)
            continue;

        for (int k = 0; k < track.keys.size(); ++k)
        {
            const Key& key = track.keys[k];

            ImVec2 keyPos = KeyToTimeLine(key);
            ImVec2 points[4] = {
                { keyPos + ImVec2(0.5,3.5)},
                { keyPos + ImVec2(3.5,0.5) },
                { keyPos + ImVec2(0.5,-2.5) },
                { keyPos + ImVec2(-2.5,0.5) }
            };

            draw_list->ChannelsSetCurrent(1);
            draw_list->AddConvexPolyFilled(points, 4, BACKGROUND_COL, false);

            ImColor col = KEY_COL;

            if ((selecting && IsPointInside(dragBegin, dragEnd, keyPos, 2.0f)) || key.selected)
                col = ImColor(255, 255, 255, 255);

            draw_list->AddPolyline(points, 4, col, true, 1.0f, true);
            draw_list->ChannelsSetCurrent(0);

            if (k+1 < track.keys.size())
            {
                ImVec2 nextKeyPos = KeyToTimeLine(track.keys[k+1]);
                // line segment
                draw_list->AddLine(keyPos, nextKeyPos, track.col, 1.0f);
            }
        }
    }
	//draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, ImColor(200, 200, 100), 3.0f);

	draw_list->ChannelsMerge();

	// Open context menu
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
	{
		node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
		open_context_menu = true;
	}
	if (open_context_menu)
	{
		ImGui::OpenPopup("context_menu");
		if (node_hovered_in_list != -1)
			node_selected = node_hovered_in_list;
		if (node_hovered_in_scene != -1)
			node_selected = node_hovered_in_scene;
	}

	// Scrolling
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
		scrolling = scrolling - ImGui::GetIO().MouseDelta;

    // Moving
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
    {
        if (ImGui::IsMouseClicked(1))
        {
            moving = true;
            moveBegin = ImGui::GetMousePos();
        }
        if (ImGui::IsMouseReleased(1))
        {
            moving = false;

        }
    }

    bool addedKey = false;
    if (ImGui::IsMouseDoubleClicked(0))
    {
        // find closest line segment
        for (int t = 0; t < tracks.size(); ++t)
        {
            Track& track = tracks[t];

            if (!track.visible)
                continue;

            for (int k = 0; k+1 < track.keys.size(); ++k)
            {
                ImVec2 start = KeyToTimeLine(track.keys[k]);
                ImVec2 end = KeyToTimeLine(track.keys[k+1]);

                if (start.x < ImGui::GetMousePos().x && end.x > ImGui::GetMousePos().x)
                {
                    float param = (ImGui::GetMousePos().x - start.x)/(end.x-start.x);
                    ImVec2 pt(start.x + param*(end.x-start.x), start.y + param*(end.y-start.y));

                    ImVec2 d = pt - ImGui::GetMousePos();

                    if ((d.x*d.x + d.y*d.y)<4.0f)
                    {
                        // subdiv track
                        Key newKey = { track.keys[k].time + param * (track.keys[k+1].time - track.keys[k].time), track.keys[k].val + param * (track.keys[k+1].val - track.keys[k].val) };
                        track.keys.insert(track.keys.begin()+k+1, newKey);
                        addedKey = true;
                        break;
                    }
                }
            }
        }
    }
    else if (ImGui::IsMouseClicked(0))
    {
        dragBegin = ImGui::GetMousePos();
        dragEnd = dragBegin;

        // clear selection
        for (int t = 0; t < tracks.size(); ++t)
        {
            Track& track = tracks[t];

            if (!track.visible)
                continue;

            for (int k = 0; k < track.keys.size(); ++k)
            {
                Key& key = track.keys[k];

                ImVec2 keyPos = KeyToTimeLine(key);
                key.selected = false;
            }
        }
    }
    else if (ImGui::IsMouseReleased(0))
    {
        // see if we selected something
        for (int t = 0; t < tracks.size(); ++t)
        {
            Track& track = tracks[t];

            if (!track.visible)
                continue;

            for (int k = 0; k < track.keys.size(); ++k)
            {
                Key& key = track.keys[k];

                ImVec2 keyPos = KeyToTimeLine(key);
                if (IsPointInside(dragBegin, dragEnd, keyPos, 2.0f))
                {
                    key.selected = true;
                }
            }
        }

        dragBegin = dragEnd;
        selecting = false;
    }

    // Selection
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(0, 2.0f))
    {
        selecting = true;
        dragEnd = ImGui::GetMousePos();
    }

    if (selecting)
    {
        draw_list->AddRect(dragBegin, dragEnd, ImColor(255, 255, 255, 255));
    }

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
	ImGui::EndGroup();

	ImGui::End();
}
int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(_T("ImGui Example"), _T("ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (CreateDeviceD3D(hwnd) < 0)
    {
        CleanupDeviceD3D();
        UnregisterClass(_T("ImGui Example"), wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup ImGui binding
    ImGui_ImplDX11_Init(hwnd, g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 14.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.45f, 0.45f, 0.45f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.65f, 0.65f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.34f, 0.44f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.22f, 0.22f, 0.22f, 0.51f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.50f, 0.64f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.44f, 0.57f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.14f, 0.44f, 0.57f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.56f, 0.72f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.44f, 0.57f, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.00f, 0.18f, 0.25f, 0.65f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.60f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 0.45f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.14f, 0.44f, 0.57f, 1.00f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 8.0f;
	style.GrabRounding = 0.0f;

    bool show_another_window = true;
    ImVec4 clear_col = ImColor(114, 144, 154);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        ImGui_ImplDX11_NewFrame();


        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
			ShowExampleAppCurveEditor(&show_another_window);
        }

        // Rendering
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_col);
        ImGui::Render();
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    CleanupDeviceD3D();
    UnregisterClass(_T("ImGui Example"), wc.hInstance);

    return 0;
}
