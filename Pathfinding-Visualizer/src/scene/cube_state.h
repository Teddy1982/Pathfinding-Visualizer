#pragma once

// Struktur für Darstellung eines Knotens (für 2D- und 3D-Darstellung)

struct alignas(16) CubeState {
	float color[4];
	float scale[4];
};
