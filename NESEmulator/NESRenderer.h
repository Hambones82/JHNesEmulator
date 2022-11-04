#pragma once

class NESRenderer {
public:
	virtual void StartFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void SetColor(int r, int g, int b, int a) = 0;
	virtual void DrawPixel(int x, int y) = 0;
};